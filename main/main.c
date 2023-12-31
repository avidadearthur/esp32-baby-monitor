#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "espnow_mic.h"
#include "espnow_send.h"
#include "espnow_recv.h"
#include "fftpeak.h"
#include "sd_record.h"
#include "u_interface.h"
#include "nrf24.h"

static const char *TAG = "main.c";

#if (RECV)
static StreamBufferHandle_t network_stream_buf; // only for reciever
#else
static StreamBufferHandle_t mic_stream_buf;
static StreamBufferHandle_t fft_stream_buf;    // only for transmitter
static StreamBufferHandle_t record_stream_buf; // only for transmitter
static StreamBufferHandle_t freq_stream_buf;   // only for transmitter
#endif

#if (UI_CONNECTED)
static StreamBufferHandle_t nrf_data_xStream;
#endif

void app_main(void)
{
    // deafult transmission rate of esp_now_send is 1Mbps = 125KBps, stream buffer size has to be larger than 125KBps
#if (!RECV)
    mic_stream_buf = xStreamBufferCreate(EXAMPLE_I2S_READ_LEN, 1);
    // check if the stream buffer is created
    if (mic_stream_buf == NULL)
    {
        printf("Error creating mic stream buffer: %d\n", errno);
        deinit_config();
        exit(errno);
    }

#if (!RECV) & (FFT_TASK)
    fft_stream_buf = xStreamBufferCreate(EXAMPLE_I2S_READ_LEN, EXAMPLE_I2S_READ_LEN / 16);
    // set trigger level to 1/16 of the stream buffer size
    xStreamBufferSetTriggerLevel(fft_stream_buf, EXAMPLE_I2S_READ_LEN / 16);
    // check if the stream buffer is created
    if (fft_stream_buf == NULL)
    {
        printf("Error creating fft stream buffer: %d\n", errno);
        deinit_config();
        exit(errno);
    }

#endif
#if (!RECV) & (FFT_TASK) & (RECORD_TASK)
    record_stream_buf = xStreamBufferCreate(EXAMPLE_I2S_READ_LEN, 1);
    freq_stream_buf = xStreamBufferCreate(EXAMPLE_I2S_READ_LEN/16, 1);
    // check if the stream buffer is created
    if (record_stream_buf == NULL || freq_stream_buf == NULL)
    {
        printf("Error creating record stream buffer: %d\n", errno);
        deinit_config();
        exit(errno);
    }
#endif

#else
    network_stream_buf = xStreamBufferCreate(BYTE_RATE, 1);
    // check if the stream buffer is created
    if (network_stream_buf == NULL)
    {
        printf("Error creating network stream buffer: %d\n", errno);
        deinit_config();
        exit(errno);
    }
#endif

#if (RECV)
    // initialize the reciever and audio (only for reciever)
    init_recv(network_stream_buf);
    init_audio_recv(network_stream_buf);
#endif

    // initialize espnow, nvm, wifi, and i2s configuration
    init_config();

#if (!RECV)
    init_transmit(mic_stream_buf);
    init_audio_trans(mic_stream_buf, fft_stream_buf, record_stream_buf);
#if (!RECV) & (FFT_TASK)
    init_fft(fft_stream_buf, freq_stream_buf);
#endif
#if (!RECV) & (FFT_TASK) & (RECORD_TASK)
    init_recording(record_stream_buf, freq_stream_buf);
#endif
#endif

#if (RECV)
#if UI_CONNECTED
    const size_t xStreamBufferSizeBytes = 100, xTriggerLevel = 1;

    nrf_data_xStream = xStreamBufferCreate(xStreamBufferSizeBytes, xTriggerLevel);
    // check if the stream buffer was created successfully
    if (nrf_data_xStream == NULL)
    {
        ESP_LOGE(TAG, "main.c - Error creating stream buffer");
    }

    /*------------------------Datetime init-------------------------------*/
    // Set the system timezone
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    // Connect to the internet and synchronize the time with an NTP server
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    /*------------------------Datetime init-------------------------------*/

    init_nrf24(nrf_data_xStream);
    init_u_interface(nrf_data_xStream);
#endif
#endif
}
