#ifndef _CEPSTRUM_H_
#define _CEPSTRUM_H_

#include <fftw3.h>

/* calculate and return the calloc cepstrum array of double */
double* cepstrum_calculate (int samples, double* signal, fftw_complex* spectrum);

/* count the occurance ofconsonant in the cepstrum */
int vowels_and_consonant_count (double* cepstrum, double* freqs);

/* destroy the plan and free the calloc */
void clean_cepstrum();

#endif  //_CEPSTRUM_H_