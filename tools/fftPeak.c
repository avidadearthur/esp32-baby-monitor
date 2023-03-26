#include <stdio.h>
#include <fftw3.h>

#define SAMPLES 1024
#define SAMPLING_RATE 16000.0
#define FREQ_STEP (SAMPLING_RATE / SAMPLES)

// fftw is a library developed by MIT: https://www.fftw.org

double adc_out[SAMPLES];

int main()
{
    double signal[SAMPLES];             /* real-valued input N */
    fftw_complex spectrum[SAMPLES/2+1]; /* only N/2 + 1 freq coef are needed (real-valued, complex conjugates) */
    double freqs[SAMPLES/2+1];          /* frequencies involved: f = n.fs/N */

    // fill signal with ADC output (example)
    for (int i = 0; i < SAMPLES; i++) {
        signal[i] = adc_out[i];
    }

    // create FFT plan
    fftw_plan plan = fftw_plan_dft_r2c_1d(SAMPLES, signal, spectrum, FFTW_ESTIMATE);

    // execute FFT
    fftw_execute(plan);

    // calculate frequencies for each bin in spectrum
    for (int i = 0; i <= SAMPLES/2; i++) {
        freqs[i] = i * FREQ_STEP;
    }

    // find peaks in spectrum
    for (int i = 1; i < SAMPLES/2; i++) {
        double power = spectrum[i][0]*spectrum[i][0] + spectrum[i][1]*spectrum[i][1];           /* amplitude sqr */
        if (power > (spectrum[i-1][0]*spectrum[i-1][0] + spectrum[i-1][1]*spectrum[i-1][1]) && 
            power > (spectrum[i+1][0]*spectrum[i+1][0] + spectrum[i+1][1]*spectrum[i+1][1])) {
            double freq = freqs[i];
            double amplitude = sqrt(power);
            printf("Peak detected at frequency %lf Hz with amplitude %lf\n", freq, amplitude);
        }
    }


    // free resources
    fftw_destroy_plan(plan);
    fftw_cleanup();
    
    return 0;
}
