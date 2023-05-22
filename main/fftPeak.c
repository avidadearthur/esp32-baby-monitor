#include "fftpeak.h"
#include "music.h"
#include "zcr.h"

#define REP 100
#define MIN_LOG_N 0
#define MAX_LOG_N 12

// define frequency step for each bin in spectrum with N = 1024, if higher stack overflow (only 160 available at the end) or heap cap occurs
#define N_SAMPLES (EXAMPLE_I2S_READ_LEN / 16) // Amount of real input samples. FFT size max in DSP is 4096. EXAMPLE_I2S_READ_LEN
#define FREQ_STEP (EXAMPLE_I2S_SAMPLE_RATE / N_SAMPLES)

#define FFT_DEBUG 0

static const char *TAG = "FFTPEAK";
TaskHandle_t fft_task_handle = NULL;
StreamBufferHandle_t freq_stream_buf;

// get task handle of fft task
TaskHandle_t get_fft_task_handle()
{
    assert(fft_task_handle != NULL);
    return fft_task_handle;
}

void fft_task(void *task_param)
{
    // get the stream buffer handle from the task parameter
    StreamBufferHandle_t fft_stream_buf = (StreamBufferHandle_t)task_param;

    int N = N_SAMPLES;                // FFT size max in DSP is 4096. EXAMPLE_I2S_READ_LEN
    int fs = EXAMPLE_I2S_SAMPLE_RATE; // sample rate

    fft_config_t *fft_analysis;
    // Create fft plan config and let it allocate input and output arrays
    fft_analysis = fft_init(N, FFT_REAL, FFT_FORWARD, NULL, NULL);

    // calculate frequencies for each bin in spectrum
    float freqs[((fft_analysis->size) / 2) + 1];
    for (int i = 0; i <= ((fft_analysis->size) / 2); i++)
    {
        if (i == 1)
        {
            freqs[i] = ((fft_analysis->size) / 2) * FREQ_STEP;
        }
        else
        {
            freqs[i] = i * FREQ_STEP;
        }
    }

    // create a power array to hold the power spectrum
    float *power = calloc((fft_analysis->size) / 2 + 1, sizeof(float));
    // check if power is allocated
    if (power == NULL)
    {
        ESP_LOGE(TAG, "power is NULL");
        exit(1);
    }

    // create a buffer of size N to hold the fft input signal
    uint8_t *fft_input = calloc(N, sizeof(char));
    // check if fft_input is allocated
    if (fft_input == NULL)
    {
        ESP_LOGE(TAG, "fft_input is NULL");
        exit(1);
    }

    size_t bytes_available = xStreamBufferBytesAvailable(fft_stream_buf);
    ESP_LOGI(TAG, "Bytes available in stream buffer: %d \n", bytes_available);
    // ticks to wait for filling fft to complete
    TickType_t wait_ticks = pdMS_TO_TICKS(1000);

#if (FFT_DEBUG)
    // declare a counter to count until processed sample == sample rate
    int count = 0;
#endif

    ESP_LOGI(TAG, "FFT task started \n");
    // when there is data available in the stream buffer, read it and execute fft
    while (true)
    {
        // fill signal with ADC output (use xStreamBufferReceive() to get data from ADC DMA buffer) with size sample rate
        size_t byte_received = xStreamBufferReceive(fft_stream_buf, fft_input, N, wait_ticks);

        // scale the 12-bit wide ADC output to 32-bit float
        for (int i = 0; i < N; i++)
        {
            fft_analysis->input[i] = (float)fft_input[i] / 4096;
        }
        // execute fft
        fft_execute(fft_analysis);
        // calculate power spectrum
        for (int i = 0; i < (fft_analysis->size) / 2; i++)
        {
            if (i == 0)
            {
                power[i] = (fft_analysis->output)[0] * (fft_analysis->output)[0]; /* DC component */
            }
            else if (i == (fft_analysis->size) / 2)
            {
                power[i] = (fft_analysis->output)[1] * (fft_analysis->output)[1]; /* Nyquist component */
            }
            else
            {
                power[i] = (fft_analysis->output)[2 * i] * (fft_analysis->output)[2 * i] + (fft_analysis->output)[(2 * i) + 1] * (fft_analysis->output)[(2 * i) + 1]; /* amplitude sqr */
            }
        }

#if (FFT_DEBUG)
        // increment count
        count++;
#endif

        float max1 = 0;
        float max2 = 0;
        float freq1 = 0;
        float freq2 = 0;

        // loop through power spectrum to find two highest peaks
        for (int i = 0; i < ((fft_analysis->size) / 2); i++)
        {
            if ((i > 5) && (i < 100))
            {
                if ((power[i] > max1))
                {
                    max2 = max1;
                    freq2 = freq1;
                    max1 = power[i];
                    freq1 = freqs[i];
                }
                else if (power[i] > max2)
                {
                    max2 = power[i];
                    freq2 = freqs[i];
                }
            }
        }
        
        #if (RECORD_TASK)
        // define a buffer to store frequency and max amplitude
            float* freq_buffer = calloc(4, sizeof(float));
            freq_buffer[0] = freq1;
            freq_buffer[1] = max1;
            freq_buffer[2] = freq2;
            freq_buffer[3] = max2;
            // 4 * sizeof(float) = 4 * 4 = 16 bytes
            xStreamBufferSend(freq_stream_buf, freq_buffer, 4*sizeof(float), portMAX_DELAY);
        #endif

        // if peak is in range of 350-550 Hz, then it is a note
        if ((freq1 > 400.0 && freq1 < 500.0) & (freq2 > 1150.0 && freq2 < 1500.0)) {

            ESP_LOGI(TAG, "note detected at f0 %lf Hz with amplitude %lf and f2 %lf with amplitude %lf\n", freq1, max1, freq2, max2);
            // if amplitude of frequency 1 is greater than 0.06, then cry score + 1
            int cry_score = 0;
            if (max1 > 0.1)
            {
                cry_score++;
            }
            // // if amplitude of frequency 2 is greater than 0.06, then cry score + 1
            if (max2 > 0.1)
            {
                cry_score++;
            }
            // db confirm if the baby is crying by calculating zcr
            bool is_cry = zcr(fft_analysis->input, N, fs);

            // // if zcr is true, then cry score + 1
            if (is_cry)
            {
                cry_score++;
            }
            // // if cry score is greater than 2 and zcr confirms that the baby is crying, then play music
            ESP_LOGI(TAG, "cry score is %d", cry_score);

            if (cry_score == 3)
            {
                ESP_LOGI(TAG, "cry detected at f0 %lf Hz with amplitude %lf and f2 %lf with amplitude %lf\n", freq1, max1, freq2, max2);
                // call the init_music functoin to play from existing audio file
                init_music(fft_task_handle);
                // get the music task handle
                TaskHandle_t music_task_handle = get_music_play_task_handle();
                // wait for the music task to finish
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                // notigy music task to resume
                xTaskNotifyGive(music_task_handle);
                ESP_LOGI(TAG, "resumed the fft task");

                // clear the buffers to avoid retriggering music player with the same signal fraction
                memset(power, 0, (fft_analysis->size) / 2 + 1);
                memset(fft_input, 0, (fft_analysis->size) / 2 + 1);
                memset(fft_analysis->output, 0, (fft_analysis->size));
                memset(fft_analysis->input, 0, (fft_analysis->size));
                // reset freq1, freq2, max1, max2, and cryscore
                freq1 = 0;
                freq1 = 0;
                max1 = 0;
                max2 = 0;
                cry_score = 0;

                // if music task handle is not null, then delete the music task
                if ((music_task_handle = get_music_play_task_handle()) != NULL)
                {
                    vTaskDelete(music_task_handle);
                    ESP_LOGI(TAG, "deleted the music task");
                    // get the music task status
                    eTaskState music_task_status = eTaskGetState(music_task_handle);
                    ESP_LOGI(TAG, "music task status: %d. 0 means running, 4 means in the delete queue but not yet deleted, 5 means invalid\n", music_task_status);
                    music_task_handle = NULL;
                }
            }
        }

#if (FFT_DEBUG)
        if ((count % (EXAMPLE_I2S_SAMPLE_RATE / N_SAMPLES) == 0) & (count > 3000))
        {
            ESP_LOGI(TAG, "peak 1 at frequency %lf Hz with amplitude %lf \n", freq1, max1);
            ESP_LOGI(TAG, "peak 2 at frequency %lf Hz with amplitude %lf \n", freq2, max2);
            count = 0;
        }
#endif
    }

    // clear stack
    vPortFree(fft_input);
    fft_input = NULL;
    vPortFree(power);
    power = NULL;
    fft_destroy(fft_analysis);
    fft_analysis = NULL;
    vTaskDelete(NULL);
}

void init_fft(StreamBufferHandle_t fft_audio_buf, StreamBufferHandle_t freq_audio_buf)
{
    // ticks to wait for filling fft to complete
    TickType_t wait_ticks = pdMS_TO_TICKS(1000);
    // create a delay
    xTaskNotifyWait(0, 0, NULL, wait_ticks);
#if (RECORD_TASK)
    freq_stream_buf = freq_audio_buf;
#endif
    // create a task to run the fft
    xTaskCreate(fft_task, "fft_task", 4096, (void *)fft_audio_buf, IDLE_TASK_PRIO, &fft_task_handle);
}
