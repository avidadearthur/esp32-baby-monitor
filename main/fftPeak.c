#include "fftpeak.h"


#define REP 100
#define MIN_LOG_N 0
#define MAX_LOG_N 12
#define FREQ_STEP (EXAMPLE_I2S_SAMPLE_RATE / EXAMPLE_I2S_READ_LEN)

void init_fft(StreamBufferHandle_t fft_stream_buf){
    
    int n;

    fft_config_t* fft_analysis;

    int NFFT = EXAMPLE_I2S_READ_LEN/16; // FFT size max in DSP is 4096. EXAMPLE_I2S_READ_LEN/16 is 1024

    // Create fft plan config and let it allocate input and output arrays
    fft_analysis = fft_init(NFFT, FFT_REAL, FFT_FORWARD, NULL, NULL);
    // ticks to wait for filling fft to complete
    TickType_t wait_ticks = pdMS_TO_TICKS(1000);
    
    // create a delay
    xTaskNotifyWait(0, 0, NULL, wait_ticks);

    // // creeate a counter to count to sample rate/fft size
    // int counter = 0;

    size_t bytes_available = xStreamBufferBytesAvailable(fft_stream_buf);
    printf("Bytes available in stream buffer: %d \n", bytes_available);
    
    // when there is data available in the stream buffer, read it and execute fft
    while (xStreamBufferBytesAvailable(fft_stream_buf)) {
        // fill signal with ADC output (use xStreamBufferReceive() to get data from ADC DMA buffer) with size sample rate
        size_t N = xStreamBufferReceive(fft_stream_buf, fft_analysis->input, NFFT, wait_ticks);
        // printf("Bytes read from stream buffer: %d \n", N);
        assert(N == NFFT);

        // increment counter
        counter++;

        // execute fft
        fft_execute(fft_analysis);

        float freqs[NFFT/2+1];

        // calculate frequencies for each bin in spectrum
        for (int i = 0; i <= (NFFT/2)+1; i++) {
            freqs[i] = i * FREQ_STEP;
        }

        // find peaks in spectrum
        for (int i = 1; i < (NFFT/2)+1; i++) {
            float power = (fft_analysis->output)[i]*(fft_analysis->output)[i] + (fft_analysis->output)[i+1]*(fft_analysis->output)[i+1];           /* amplitude sqr */
            if (power > ((fft_analysis->output)[i-2]*(fft_analysis->output)[i-2] + (fft_analysis->output)[i-1]*(fft_analysis->output)[i-1]) && 
                power > ((fft_analysis->output)[i+2]*(fft_analysis->output)[i+2] + (fft_analysis->output)[i+3]*(fft_analysis->output)[i+3])) {
                float freq = freqs[i];
                float amplitude = sqrt(power);
                printf("Peak detected at frequency %lf Hz with amplitude %lf \n", freq, amplitude);
                
                // if peak is in range of 350-550 Hz, then it is a note
                if (freq > 350 && freq < 550) {
                    printf("f0 detected at frequency %lf Hz with amplitude %lf \n", freq, amplitude);
                }
                // if peak is in range 1150 - 1500 Hz, then it is a note
                else if (freq > 1150 && freq < 1500) {
                    printf("cry detected at frequency %lf Hz with amplitude %lf \n", freq, amplitude);
                }

            }
        }

        // // if counter is equal to sample rate/fft size, then reset counter
        // if (counter == EXAMPLE_I2S_SAMPLE_RATE/NFFT) {
        //     counter = 0;
        //     int score = 0;
        //     // iterate through 
        // }

    }

    // free resources
    fft_destroy(fft_analysis);
}
