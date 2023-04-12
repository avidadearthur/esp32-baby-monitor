#include "fftpeak.h"
#include "music.h"

#define REP 100
#define MIN_LOG_N 0
#define MAX_LOG_N 12

// define frequency step for each bin in spectrum with N = 1024, if higher stack overflow (only 160 available at the end) or heap cap occurs
#define N_SAMPLES (EXAMPLE_I2S_READ_LEN/16) // Amount of real input samples. FFT size max in DSP is 4096. EXAMPLE_I2S_READ_LEN
#define FREQ_STEP (EXAMPLE_I2S_SAMPLE_RATE / N_SAMPLES)

#define FFT_DEBUG 0
#define FFT_ESP_DSP 0

static const char* TAG = "FFTPEAK";


void fft_task(void* task_param){
    // get the stream buffer handle from the task parameter
    StreamBufferHandle_t fft_stream_buf = (StreamBufferHandle_t) task_param;

    int N = N_SAMPLES; // FFT size max in DSP is 4096. EXAMPLE_I2S_READ_LEN

#if (FFT_ESP_DSP)
    int power_of_two = dsp_power_of_two(N);
    // total sample is next power of two of N
    int total_samples = pow(2, power_of_two+1);
    // Input test array
    float* x1 = (float*) calloc(total_samples, sizeof(float));
    // Window coefficients
    // float wind[N];

    esp_err_t ret;

    ESP_LOGI(TAG, "Start Example.");
    ret = dsps_fft2r_init_fc32(NULL, total_samples);
    if (ret  != ESP_OK)
    {
        ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
        exit(1);
    }
    // Generate hann window
    // dsps_wind_hann_f32(wind, N);

#else
    fft_config_t* fft_analysis;
    // Create fft plan config and let it allocate input and output arrays
    fft_analysis = fft_init(N, FFT_REAL, FFT_FORWARD, NULL, NULL);
#endif
    size_t bytes_available = xStreamBufferBytesAvailable(fft_stream_buf);
    printf("Bytes available in stream buffer: %d \n", bytes_available);
    // ticks to wait for filling fft to complete
    TickType_t wait_ticks = pdMS_TO_TICKS(1000);

#if(!FFT_ESP_DSP)
    // calculate frequencies for each bin in spectrum
    float freqs[((fft_analysis->size)/2)+1];
    for (int i = 0; i <= ((fft_analysis->size)/2); i++) {
        if (i == 1){
            freqs[i] = (N/2)*FREQ_STEP;
        }
        else{
            freqs[i] = i*FREQ_STEP;
        }
    }
#else
    float* freqs = calloc((total_samples/2)+1, sizeof(float));
    // check if freqs is allocated
    if (freqs == NULL){
        ESP_LOGE(TAG, "freqs is NULL");
        exit(1);
    }
    for (int i = 0; i <= ((total_samples/2)); i++) {
        freqs[i] = i*FREQ_STEP;
    }
#endif

    // create a buffer of size N to hold the fft input signal
    uint8_t* fft_input = calloc(N, sizeof(uint8_t));
    // check if fft_input is allocated
    if (fft_input == NULL){
        ESP_LOGE(TAG, "fft_input is NULL");
        exit(1);
    }

#if (!FFT_ESP_DSP)
    // create a power array to hold the power spectrum
    float* power = calloc((fft_analysis->size)/2+1, sizeof(float));
    // check if power is allocated
    if (power == NULL){
        ESP_LOGE(TAG, "power is NULL");
        exit(1);
    }
#endif

#if (FFT_DEBUG)
    // create a timer to coutn the time elapsed
    // time_t start_time = time(NULL);
    int count = 0;
#endif

    // when there is data available in the stream buffer, read it and execute fft
    while (true) {
        
        // fill signal with ADC output (use xStreamBufferReceive() to get data from ADC DMA buffer) with size sample rate
        size_t byte_received = xStreamBufferReceive(fft_stream_buf, fft_input, N, wait_ticks);
        assert(byte_received == N);

    #if (!FFT_ESP_DSP)
        // scale the 12-bit wide ADC output to 32-bit float
        for (int i = 0; i < N; i++) {
            fft_analysis->input[i] = (float)fft_input[i] / 4096;
        }
        // execute fft
        fft_execute(fft_analysis);
        // calculate power spectrum
        for (int i = 0; i < (fft_analysis->size)/2; i++) {
            if(i == 0){
                power[i] = (fft_analysis->output)[0]*(fft_analysis->output)[0];   /* DC component */
            }
            else if(i == (fft_analysis->size)/2){
                power[i] = (fft_analysis->output)[1]*(fft_analysis->output)[1];   /* Nyquist component */
            }
            else{
                power[i] = (fft_analysis->output)[2*i]*(fft_analysis->output)[2*i] + (fft_analysis->output)[(2*i)+1]*(fft_analysis->output)[(2*i)+1];   /* amplitude sqr */
            }
        }
    #else
        // memory copy the input signal to x1 align to 32-bit
        memcpy(x1, fft_input, N*sizeof(char));
        // FFT
        dsps_fft2r_fc32(x1, total_samples);
        // Bit reversal
        dsps_bit_rev_fc32(x1, total_samples);
        // Convert one complex vector with length N to one real specturm vector with length M
        dsps_cplx2reC_fc32(x1, total_samples);

        for (int i = 0 ; i < total_samples/2 ; i++) {
            x1[i] = 10 * log10f((x1[i * 2 + 0] * x1[i * 2 + 0] + x1[i * 2 + 1] * x1[i * 2 + 1] + 0.0000001));
        }

    #endif
    
    #if (FFT_DEBUG)
        // increment count
        count++;
    #endif

    float max1 = 0;
    float max2 = 0;
    float freq1 = 0;
    float freq2 = 0;

    #if(!FFT_ESP_DSP)
        // loop through power spectrum to find two highest peaks
        for (int i = 0; i < ((fft_analysis->size)/2); i++) {

            if((i > 5) && (i < 100)){
                if ((power[i] > max1)) {
                    max2 = max1;
                    freq2 = freq1;
                    max1 = power[i];
                    freq1 = freqs[i];
                }
                else if (power[i] > max2) {
                    max2 = power[i];
                    freq2 = freqs[i];
                }
            }

        }

        // if peak is in range of 350-550 Hz, then it is a note
        if ((freq1 > 350.0 && freq1 < 500.0) & (freq2 > 1150.0 && freq2 < 1500.0)) {
            printf("cry detected at f0 %lf Hz with amplitude %lf and f2 %lf with amplitude %lf\n", freq1, max1, freq2, max2);
            // call the init_music functoin to play from existing audio file
            // init_music();
        }

        #if(FFT_DEBUG)
            // // check if the timer has reached 600 second
            // if (time(NULL) - start_time >= 60*1000) {
            //     // reset the timer
            //     start_time = time(NULL);
            //     // break the loop
            //     printf("breaking fft loop \n");
            //     // display the frequency and amplitude of the two highest peaks
            //     break;
            // }
            // if the remainder of count divided by x is 0, then print the frequency and amplitude of the two highest peaks

            if ((count % (EXAMPLE_I2S_SAMPLE_RATE/N_SAMPLES) == 0) & (count > 30)) {
                printf("peak 1 at frequency %lf Hz with amplitude %lf \n", freq1, max1);
                printf("peak 2 at frequency %lf Hz with amplitude %lf \n", freq2, max2);

                count = 0;
            }


        #endif

    #endif
    #if (FFT_ESP_DSP)
        // find the peaks in the power spectrum and their corresponding frequencies
        for (int i = 0; i < total_samples/2; i++) {
            if(i > 4 && i < 100){
                if ((x1[i] > max1)) {
                    max2 = max1;
                    freq2 = freq1;
                    max1 = x1[i];
                    freq1 = freqs[i];
                }
                else if (x1[i] > max2) {
                    max2 = x1[i];
                    freq2 = freqs[i];
                }
            }
        }
        #if(FFT_DEBUG)
        // // check if the timer has reached 600 second
        // if (time(NULL) - start_time >= 60*1000) {
        //     // reset the timer
        //     start_time = time(NULL);
        //     // break the loop
        //     printf("breaking fft loop \n");
        //     // display the frequency and amplitude of the two highest peaks
        //     break;
        // }
        // if the remainder of count divided by x is 0, then print the frequency and amplitude of the two highest peaks

        if (count % (EXAMPLE_I2S_SAMPLE_RATE/N_SAMPLES) == 0) {
            printf("peak 1 at frequency %lf Hz with amplitude %lf \n", freq1, max1);
            printf("peak 2 at frequency %lf Hz with amplitude %lf \n", freq2, max2);

            count = 0;
        }
        #endif

    #endif
    }
    
    // clear stack
    vPortFree(fft_input);
    fft_input = NULL;
#if(!FFT_ESP_DSP)
    vPortFree(power);
    power = NULL;
    fft_destroy(fft_analysis);
    fft_analysis = NULL;
#else
    vPortFree(x1);
    x1 = NULL;
    vPortFree(freqs);
    freqs = NULL;
#endif
    vTaskDelete(NULL);
}

void init_fft(StreamBufferHandle_t fft_audio_buf){
    // ticks to wait for filling fft to complete
    TickType_t wait_ticks = pdMS_TO_TICKS(1000);
    // create a delay
    xTaskNotifyWait(0, 0, NULL, wait_ticks);
    // create a task to run the fft
    xTaskCreate(fft_task, "fft_task", 4096, (void*) fft_audio_buf, 4, NULL);
}






