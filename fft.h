
#ifndef SALVOQ_FFT_H
#define SALVOQ_FFT_H
#include <stddef.h>
void fft_init(size_t n);
void fft_forward(float *re, float *im, size_t n);
void fft_free(void);
#endif
