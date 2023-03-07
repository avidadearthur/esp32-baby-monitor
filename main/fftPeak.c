#include <stdio.h>
#include <fastmath.h>
#include <complex.h>
#include "config.h"
#include "fft.h"


#define REP 100
#define MIN_LOG_N 0
#define MAX_LOG_N 12
#define SAMPLES 1024
#define SAMPLING_RATE 16000
#define FREQ_STEP (SAMPLING_RATE / SAMPLES)

float fft_peak(StreamBufferHandle_t mic_stream_buf){
    
    int n;

    fft_config_t* fft_analysis;

    for (n = MIN_LOG_N ; n <= MAX_LOG_N ; n++){

        int NFFT = 1 << n;

        // Create fft plan and let it allocate arrays
        fft_analysis = fft_init(NFFT, FFT_REAL, FFT_FORWARD, NULL, NULL);
        // ticks to wait for filling fft to complete
        TickType_t wait_ticks = pdMS_TO_TICKS(1000);
        // fill signal with ADC output (use xStreamBufferReceive() to get data from ADC DMA buffer) with size sample rate
        size_t N = xStreamBufferReceive(mic_stream_buf, fft_analysis->input, NFFT, wait_ticks);
        assert(N == SAMPLES);
        // execute fft
        fft_execute(fft_analysis);

    }

    float freqs[SAMPLES/2+1];

    // calculate frequencies for each bin in spectrum
    for (int i = 0; i <= SAMPLES/2; i++) {
        freqs[i] = i * FREQ_STEP;
    }

    // find peaks in spectrum
    for (int i = 1; i < SAMPLES/2; i++) {
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

    return 0;
}
