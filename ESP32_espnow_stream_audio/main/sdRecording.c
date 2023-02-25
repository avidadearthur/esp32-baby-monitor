#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

#include "color_out.h"
#include "audio.h"
#include "sdRecording.h"

/** TODO: Move to separate file .c */
// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.
sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t *card;
const int WAVE_HEADER_SIZE = 44;
static const char *TAG = "sdRecording.c";
/** ------------------------------------------------------------------*/

/** TODO: Move to separate file .c */
/**
 * @brief Initializes the slot without card detect (CD) and write protect (WP) signals.
 * It formats the card if mount fails and initializes the card. After the card has been
 * initialized, it print the card properties
 */
void mount_sdcard(void)
{
    esp_err_t ret;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 8 * 128};
    ESP_LOGI(TAG, "Initializing SD card");

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_MOSI_GPIO,
        .miso_io_num = SPI_MISO_GPIO,
        .sclk_io_num = SPI_CLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SPI_CS_GPIO;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return;
    }

    sdmmc_card_print_info(stdout, card); // Card has been initialized, print its properties
}
/** ------------------------------------------------------------------------------------------*/

/** TODO: Move to separate file .c */
/**
 * @brief Generates the header for the WAV file that is going to be stored in the SD card.
 * See this for reference: http://soundfile.sapp.org/doc/WaveFormat/.
 */
void generate_wav_header(char *wav_header, uint32_t wav_size, uint32_t sample_rate)
{
    uint32_t file_size = wav_size + WAVE_HEADER_SIZE - 8;
    uint32_t byte_rate = BYTE_RATE;

    const char set_wav_header[] = {
        'R', 'I', 'F', 'F',                                                  // ChunkID
        file_size, file_size >> 8, file_size >> 16, file_size >> 24,         // ChunkSize
        'W', 'A', 'V', 'E',                                                  // Format
        'f', 'm', 't', ' ',                                                  // Subchunk1ID
        0x10, 0x00, 0x00, 0x00,                                              // Subchunk1Size (16 for PCM)
        0x01, 0x00,                                                          // AudioFormat (1 for PCM)
        0x01, 0x00,                                                          // NumChannels (1 channel)
        sample_rate, sample_rate >> 8, sample_rate >> 16, sample_rate >> 24, // SampleRate
        byte_rate, byte_rate >> 8, byte_rate >> 16, byte_rate >> 24,         // ByteRate
        0x02, 0x00,                                                          // BlockAlign
        0x10, 0x00,                                                          // BitsPerSample (16 bits)
        'd', 'a', 't', 'a',                                                  // Subchunk2ID
        wav_size, wav_size >> 8, wav_size >> 16, wav_size >> 24,             // Subchunk2Size
    };

    memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}
/** ------------------------------------------------------------------------------------------*/

void rec_task(void *arg)
{
    /** TODO: Move to separate function*/
    mount_sdcard();

    // Use POSIX and C standard library functions to work with files.
    int flash_wr_size = 0;
    int rec_time = 10; // seconds

    char wav_header_fmt[WAVE_HEADER_SIZE];

    uint32_t flash_rec_size = BYTE_RATE * rec_time;
    generate_wav_header(wav_header_fmt, flash_rec_size, I2S_SAMPLE_RATE);

    // log the size of the recording
    ESP_LOGI(TAG, "Opening file - recording size: %d", flash_rec_size);

    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(SD_MOUNT_POINT "/record.wav", &st) == 0)
    {
        // Delete it if it exists
        unlink(SD_MOUNT_POINT "/record.wav");
    }

    // Create new WAV file
    FILE *f = fopen(SD_MOUNT_POINT "/record.wav", "a");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        vTaskDelete(NULL);
    }

    // Write the header to the WAV file
    fwrite(wav_header_fmt, 1, WAVE_HEADER_SIZE, f);
    /** -------------------------------------------------------*/

    uint8_t *ucRxData = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));
    size_t xReceivedBytes;
    const TickType_t xBlockTime = pdMS_TO_TICKS(20);

    StreamBufferHandle_t xStreamBufferRec = (StreamBufferHandle_t)arg;

    color_printf(COLOR_PRINT_GREEN, "\t\trec_task: starting to listen");

    while (flash_wr_size < flash_rec_size)
    {
        /* Receive up to another sizeof( ucRxData ) bytes from the stream buffer.
        Wait in the Blocked state (so not using any CPU processing time) for a
        maximum of 100ms for the full sizeof( ucRxData ) number of bytes to be
        available. */
        xReceivedBytes = xStreamBufferReceive(xStreamBufferRec,
                                              (void *)ucRxData,
                                              READ_BUF_SIZE_BYTES * sizeof(char),
                                              xBlockTime * 2);

        if (xReceivedBytes > 0)
        {
            /* A ucRxData contains another xRecievedBytes bytes of data, which can
            be processed here.... */
            // color_printf(COLOR_PRINT_GREEN, "\t\ttx_task: notify %d bytes received", xReceivedBytes);
            //  print the received data
            //  Write the samples to the WAV file
            fwrite(ucRxData, 1, xReceivedBytes, f);
            flash_wr_size += xReceivedBytes;
            // Log the amount of bytes and the percentage of the recording
            ESP_LOGI(TAG, "Wrote %d %d/%d bytes to file - %d%%", xReceivedBytes, flash_wr_size, flash_rec_size, (flash_wr_size * 100) / flash_rec_size);
        }
        else
        {
            /* The call to xStreamBufferReceive() timed out before any data was
            available. */
            color_printf(COLOR_PRINT_RED, "\t\rec_task: notify timeout");
        }
    }
    ESP_LOGI(TAG, "Recording done!");
    fclose(f);
    ESP_LOGI(TAG, "File written on SDCard");

    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    ESP_LOGI(TAG, "Card unmounted");
    // Deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);
    // delete task after recording
    vTaskDelete(NULL);
}
/** --------------------------------------------------------------------------------------------------*/

void init_recording(StreamBufferHandle_t xStreamBufferRec)
{
    ESP_LOGI(TAG, "Starting recording task");
    xTaskCreate(rec_task, "rec_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBufferRec, 5, NULL);
}