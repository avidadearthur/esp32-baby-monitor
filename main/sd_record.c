#include "sd_record.h"

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.
sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t *card;
const int WAVE_HEADER_SIZE = 44;
static const char *TAG = "sdRecording.c";

uint8_t *audio_output_buf;

StreamBufferHandle_t freq_stream_data;

// for the rec button
// for the rec button
#define REC_BUTTON_GPIO 34
#define BUTTON_DEBOUNCE_TIME_MS 300

// SemaphoreHandle_t xSemaphore;

bool interrupt_flag = false;
uint32_t last_button_isr_time = 0;

void IRAM_ATTR rec_button_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    uint32_t isr_time = xTaskGetTickCountFromISR();

    if ((isr_time - last_button_isr_time) > pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS))
    {
        // Log rec button was pressed
        ESP_LOGI(TAG, "Rec button pressed");

        // xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

        last_button_isr_time = isr_time; // Save last ISR time
    }
}

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

void rec_and_read_task(void *task_param)
{
    /** -------------------------------------------------------*/
    mount_sdcard();
    // Use POSIX and C standard library functions to work with files.
    int flash_wr_size = 0;
    int rec_time = 10; // seconds
    char wav_header_fmt[WAVE_HEADER_SIZE];
    uint32_t flash_rec_size = BYTE_RATE * rec_time;
    generate_wav_header(wav_header_fmt, flash_rec_size, EXAMPLE_I2S_SAMPLE_RATE);

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

    // Create test .csv file
    FILE *csv = fopen(SD_MOUNT_POINT "/test.csv", "w+");
    // write the header column names into the csv file
    fprintf(csv, "freq1,max1,freq2,max2\n");

    if (csv == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        vTaskDelete(NULL);
    }

    StreamBufferHandle_t rec_stream_buf = (StreamBufferHandle_t)task_param;

    // allocate memory for the read buffer
    audio_output_buf = (uint8_t *)calloc(BYTE_RATE / 4, sizeof(char));
    // check if memory allocation was successful
    if (audio_output_buf == NULL)
    {
        ESP_LOGE(TAG, "Error allocating memory for audio output buffer");
        vTaskDelete(NULL);
    }
    float *freq_output_buf = (float *)calloc(4, sizeof(float));
    // check if memory allocation was successful
    if (freq_output_buf == NULL)
    {
        ESP_LOGE(TAG, "Error allocating memory for freq output buffer");
        vTaskDelete(NULL);
    }

    while (flash_wr_size < flash_rec_size)
    {
        size_t num_bytes = xStreamBufferReceive(rec_stream_buf, (char *)audio_output_buf, BYTE_RATE / 4, portMAX_DELAY);
        size_t peak_data = xStreamBufferReceive(freq_stream_data, (float *)freq_output_buf, 4 * sizeof(float), portMAX_DELAY);
        if (num_bytes > 0)
        {
            ESP_LOGI(TAG, "Read %d bytes from rec_stream_buf", num_bytes);
            fwrite(audio_output_buf, 1, num_bytes, f);

            fprintf(csv, "%f,%f,%f,%f\n", freq_output_buf[0], freq_output_buf[1], freq_output_buf[2], freq_output_buf[3]);
            flash_wr_size += num_bytes;
            ESP_LOGI(TAG, "Wrote %d/%ld bytes to file - %ld%%", flash_wr_size, flash_rec_size, (flash_wr_size * 100) / flash_rec_size);
            // check stack high watermark after writing to the stream buffer
            // ESP_LOGI(TAG, "Stack high watermark after writing to audio buffer: %d", stack_high_watermark);
        }
        else
        {
            ESP_LOGE(TAG, "Error reading %d bytes from rec_stream_buf", num_bytes);
        }
    }
    ESP_LOGI(TAG, "Recording done!");
    fclose(f);
    fclose(csv);
    ESP_LOGI(TAG, "File written on SDCard");

    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    ESP_LOGI(TAG, "Card unmounted");
    // Deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);
    // delete task after recording
    vTaskDelete(NULL);
}

void init_recording(StreamBufferHandle_t xStreamBufferRec, StreamBufferHandle_t xStreamBufferFreq)
{
    ESP_LOGI(TAG, "Starting recording task");
    // Initialize semaphore
    // xSemaphore = xSemaphoreCreateBinary();

    // // check the stack remaining space before creating the task
    // UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
    // ESP_LOGI(TAG, "Stack high water mark before cofiguring gpio: %d", stack_high_water_mark);

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = (1ULL << REC_BUTTON_GPIO);
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    // gpio_isr_handler_add(REC_BUTTON_GPIO, rec_button_isr_handler, NULL);

    // check the stack remaining space after creating the task
    // stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
    // ESP_LOGI(TAG, "Stack high water mark after cofiguring gpio: %d", stack_high_water_mark);

    // force if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) to be true
    // xSemaphoreGive(xSemaphore);

    // check if xStreamBufferRec is null
    if (xStreamBufferRec == NULL || xStreamBufferFreq == NULL)
    {
        ESP_LOGE(TAG, "sd_record.c - Error receiving xStreamBufferRec");
    }
    else
    {
        freq_stream_data = xStreamBufferFreq;
        xTaskCreate(rec_and_read_task, "rec_and_read_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBufferRec, IDLE_TASK_PRIO, NULL);
    }
}