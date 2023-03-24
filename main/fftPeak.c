#include "fftpeak.h"


#define REP 100
#define MIN_LOG_N 0
#define MAX_LOG_N 12
#define FFT_DEBUG 1
// define frequency step for each bin in spectrum with N = 1024, if higher stack overflow (only 160 available at the end) or heap cap occurs
#define FREQ_STEP (EXAMPLE_I2S_SAMPLE_RATE / (EXAMPLE_I2S_READ_LEN/16))

void init_fft(StreamBufferHandle_t fft_stream_buf){

    fft_config_t* fft_analysis;

    int NFFT = EXAMPLE_I2S_READ_LEN/16; // FFT size max in DSP is 4096. EXAMPLE_I2S_READ_LEN

    // Create fft plan config and let it allocate input and output arrays
    fft_analysis = fft_init(NFFT, FFT_REAL, FFT_FORWARD, NULL, NULL);
    // ticks to wait for filling fft to complete
    TickType_t wait_ticks = pdMS_TO_TICKS(1000);
    
    // create a delay
    xTaskNotifyWait(0, 0, NULL, wait_ticks);


    size_t bytes_available = xStreamBufferBytesAvailable(fft_stream_buf);
    printf("Bytes available in stream buffer: %d \n", bytes_available);

    // calculate frequencies for each bin in spectrum
    float freqs[NFFT/2+1];
    for (int i = 0; i <= (NFFT/2)+1; i++) {
        freqs[i] = i * FREQ_STEP;
    }

    // // declare variables to hold the five highest peaks and their frequencies
    // float max1 = 0.0;
    // float max2 = 0.0;
    // float max3 = 0.0;
    // float max4 = 0.0;
    // float max5 = 0.0;

    // float freq1 = 0.0;
    // float freq2 = 0.0;
    // float freq3 = 0.0;
    // float freq4 = 0.0;
    // float freq5 = 0.0;

    #if (FFT_DEBUG)
    // create a timer to coutn the time elapsed
    time_t start_time = time(NULL);
    int count = 0;
    #endif
    // when there is data available in the stream buffer, read it and execute fft
    while (xStreamBufferBytesAvailable(fft_stream_buf)) {

        // create a buffer of size NFFT to hold the fft input signal
        uint8_t* fft_input = calloc(NFFT, sizeof(uint8_t));

        // fill signal with ADC output (use xStreamBufferReceive() to get data from ADC DMA buffer) with size sample rate
        size_t N = xStreamBufferReceive(fft_stream_buf, fft_input, NFFT, wait_ticks);
        assert(N == NFFT);

        // scale the signal from 12 bit width adc data to 32 bit float
        for (int i = 0; i < NFFT; i++) {
            fft_analysis->input[i] = (float) fft_input[i] / 4096.0;
        }

        // execute fft
        fft_execute(fft_analysis);

        // increment count
        count++;

        // create a power array to hold the power spectrum
        float* power = calloc(NFFT/2+1, sizeof(float));

        // calculate power spectrum
        for (int i = 0; i < (NFFT/2)+1; i++) {
            power[i] = (fft_analysis->output)[i]*(fft_analysis->output)[i] + (fft_analysis->output)[i+1]*(fft_analysis->output)[i+1];           /* amplitude sqr */
        }

        // // loop through the power spectrum to find the five highest peaks
        // for (int i = 0; i < (NFFT/2)+1; i++) {
        //     // if the current peak is higher than the first peak, then shift the other peaks down and set the current peak as the first peak
        //     if (power[i] > max1) {
        //         max5 = max4;
        //         max4 = max3;
        //         max3 = max2;
        //         max2 = max1;
        //         max1 = power[i];
        //         freq5 = freq4;
        //         freq4 = freq3;
        //         freq3 = freq2;
        //         freq2 = freq1;
        //         freq1 = freqs[i];
        //     }
        //     // if the current peak is higher than the second peak, then shift the other peaks down and set the current peak as the second peak
        //     else if (power[i] > max2) {
        //         max5 = max4;
        //         max4 = max3;
        //         max3 = max2;
        //         max2 = power[i];
        //         freq5 = freq4;
        //         freq4 = freq3;
        //         freq3 = freq2;
        //         freq2 = freqs[i];
        //     }
        //     // if the current peak is higher than the third peak, then shift the other peaks down and set the current peak as the third peak
        //     else if (power[i] > max3) {
        //         max5 = max4;
        //         max4 = max3;
        //         max3 = power[i];
        //         freq5 = freq4;
        //         freq4 = freq3;
        //         freq3 = freqs[i];
        //     }
        //     // if the current peak is higher than the fourth peak, then shift the other peaks down and set the current peak as the fourth peak
        //     else if (power[i] > max4) {
        //         max5 = max4;
        //         max4 = power[i];
        //         freq5 = freq4;
        //         freq4 = freqs[i];
        //     }
        //     // if the current peak is higher than the fifth peak, then set the current peak as the fifth peak
        //     else if (power[i] > max5) {
        //         max5 = power[i];
        //         freq5 = freqs[i];
        //     }
        // }

                // loop through power spectrum to find two highest peaks
        float max1 = 0;
        float max2 = 0;
        float freq1 = 0;
        float freq2 = 0;
        for (int i = 0; i < (NFFT/2)+1; i++) {
            if (power[i] > max1) {
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


        // if peak is in range of 350-550 Hz, then it is a note
        if (freq1 > 350.0 && freq1 < 550.0) {
            printf("f0 detected at frequency %lf Hz with amplitude %lf \n", freq1, max1);
        }
        // if peak is in range 1150 - 1500 Hz, then it is a note
        else if (freq2 > 1150.0 && freq2 < 1500.0) {
            printf("cry detected at frequency %lf Hz with amplitude %lf \n", freq2, max2);
        }

        // clear stack
        vPortFree(fft_input);
        fft_input = NULL;
        vPortFree(power);
        power = NULL;

        #if(FFT_DEBUG)
            // check if the timer has reached 600 second
            if (time(NULL) - start_time >= 60*1000) {
                // reset the timer
                start_time = time(NULL);
                // break the loop
                printf("breaking fft loop \n");
                // display the frequency and amplitude of the two highest peaks
                break;
            }
            // if the remainder of count divided by 16 is 0, then print the frequency and amplitude of the two highest peaks
            if (count % 44 == 0) {
                printf("count == 44x, breaking fft loop \n");
                printf("peak 1 at frequency %lf Hz with amplitude %lf \n", freq1, max1);
                printf("peak 2 at frequency %lf Hz with amplitude %lf \n", freq2, max2);
                // printf("peak 3 at frequency %lf Hz with amplitude %lf \n", freq3, max3);
                // printf("peak 4 at frequency %lf Hz with amplitude %lf \n", freq4, max4);
                // printf("peak 5 at frequency %lf Hz with amplitude %lf \n", freq5, max5);

                // reset the counter
                count = 0;
            }

        #endif
    }

    // free resources
    fft_destroy(fft_analysis);
}


