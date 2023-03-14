#include "fftpeak.h"


#define REP 100
#define MIN_LOG_N 0
#define MAX_LOG_N 12
#define FREQ_STEP (EXAMPLE_I2S_SAMPLE_RATE / EXAMPLE_I2S_READ_LEN)

void init_fft(StreamBufferHandle_t fft_stream_buf){
    
    int n;

    fft_config_t* fft_analysis;

    for (n = MIN_LOG_N ; n <= MAX_LOG_N ; n++){

        int NFFT = 1 << n;

        // Create fft plan and let it allocate arrays
        fft_analysis = fft_init(NFFT, FFT_REAL, FFT_FORWARD, NULL, NULL);
        // ticks to wait for filling fft to complete
        TickType_t wait_ticks = pdMS_TO_TICKS(1000);
        // fill signal with ADC output (use xStreamBufferReceive() to get data from ADC DMA buffer) with size sample rate
        size_t N = xStreamBufferReceive(fft_stream_buf, fft_analysis->input, NFFT, wait_ticks);
        assert(N == EXAMPLE_I2S_READ_LEN);
        // execute fft
        fft_execute(fft_analysis);

    }

    float freqs[EXAMPLE_I2S_READ_LEN/2+1];

    // calculate frequencies for each bin in spectrum
    for (int i = 0; i <= EXAMPLE_I2S_READ_LEN/2; i++) {
        freqs[i] = i * FREQ_STEP;
    }

    // find peaks in spectrum
    for (int i = 1; i < EXAMPLE_I2S_READ_LEN/2; i++) {
        float power = (fft_analysis->output)[i]*(fft_analysis->output)[i] + (fft_analysis->output)[i+1]*(fft_analysis->output)[i+1];           /* amplitude sqr */
        if (power > ((fft_analysis->output)[i-2]*(fft_analysis->output)[i-2] + (fft_analysis->output)[i-1]*(fft_analysis->output)[i-1]) && 
            power > ((fft_analysis->output)[i+2]*(fft_analysis->output)[i+2] + (fft_analysis->output)[i+3]*(fft_analysis->output)[i+3])) {
            float freq = freqs[i];
            float amplitude = sqrt(power);
            printf("Peak detected at frequency %lf Hz with amplitude %lf\n", freq, amplitude);
        }
    }


    // free resources
    fft_destroy(fft_analysis);
}
