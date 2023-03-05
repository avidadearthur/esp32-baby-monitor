#include <stdio.h>
#include <fftw3.h>
#include <math.h>

#define SAMPLES 1024
#define SAMPLING_RATE 16000.0
#define FREQ_STEP (SAMPLING_RATE / SAMPLES)

// pre-defined frequency ranges for different phonemes
#define VOWEL_RANGE_START 400.0
#define VOWEL_RANGE_END 1200.0
#define CONSONANT_RANGE_START 2000.0
#define CONSONANT_RANGE_END 4000.0

double cepstrum[SAMPLES];
double* cc;
fftw_plan plan2;

double* cepstrum_calculate (int samples, double* signal, fftw_complex* spectrum){
    // take logarithm of the magnitude of the spectrum
    for (int i = 0; i <= SAMPLES/2; i++) {
        spectrum[i][0] = log10(1 + sqrt(spectrum[i][0]*spectrum[i][0] + spectrum[i][1]*spectrum[i][1]));
        spectrum[i][1] = 0.0;
    }

    // create IFFT plan
    plan2 = fftw_plan_dft_c2r_1d(SAMPLES, spectrum, cepstrum, FFTW_ESTIMATE);

    // execute IFFT
    fftw_execute(plan2);

    // calculate quefrency for each bin in cepstrum
    double quef_step = 1 / SAMPLING_RATE;
    for (int i = 0; i < SAMPLES; i++) {
        cepstrum[i] *= pow(-1.0, i);
        cepstrum[i] /= SAMPLES;
        cepstrum[i] = 20 * log10(fabs(cepstrum[i]));
        cepstrum[i] = i * quef_step;
    }
    cc = calloc(SAMPLES, sizeof(double));
    cc = cepstrum;

    return cc;

}

int vowels_and_consonant_count (double* cepstrum, double* freqs){
    int count = 0;
    // identify phonemes based on the peaks in the cepstrum
    for (int i = 0; i < SAMPLES/2; i++) {
        if (cepstrum[i] >= VOWEL_RANGE_START && cepstrum[i] <= VOWEL_RANGE_END) {
            printf("Vowel found at %lf s\n", freqs[i]);
        } 
        else if (cepstrum[i] >= CONSONANT_RANGE_START && cepstrum[i] <= CONSONANT_RANGE_END){
            printf("Consonant found at %lf s\n", freqs[i]);
            count++;
        }
    }
    return count;
}

void clean_cepstrum(){
    fftw_destroy_plan(plan2);
    free(cc);
}
