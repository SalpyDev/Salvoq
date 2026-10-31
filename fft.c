
#include "fft.h"
#include "common.h"
#include <math.h>
#include <stdlib.h>

static size_t fft_n;
static float *cos_tab, *sin_tab;
static size_t *rev;

void fft_init(size_t n) {
    fft_n = n;
    cos_tab = malloc(sizeof(float) * n / 2);
    sin_tab = malloc(sizeof(float) * n / 2);
    rev = malloc(sizeof(size_t) * n);
    for (size_t i = 0; i < n / 2; i++) {
        double a = -2.0 * SALVOQ_PI * (double)i / (double)n;
        cos_tab[i] = (float)cos(a);
        sin_tab[i] = (float)sin(a);
    }
    int bits = 0;
    while ((size_t)1 << bits < n) bits++;
    for (size_t i = 0; i < n; i++) {
        size_t r = 0;
        for (int b = 0; b < bits; b++) if (i & ((size_t)1 << b)) r |= (size_t)1 << (bits - 1 - b);
        rev[i] = r;
    }
}

void fft_forward(float *re, float *im, size_t n) {
    for (size_t i = 0; i < n; i++) {
        size_t j = rev[i];
        if (j > i) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        size_t half = len / 2, step = n / len;
        for (size_t i = 0; i < n; i += len) {
            for (size_t j = 0; j < half; j++) {
                size_t k = j * step;
                float wr = cos_tab[k], wi = sin_tab[k];
                float xr = re[i + j + half] * wr - im[i + j + half] * wi;
                float xi = re[i + j + half] * wi + im[i + j + half] * wr;
                re[i + j + half] = re[i + j] - xr;
                im[i + j + half] = im[i + j] - xi;
                re[i + j] += xr;
                im[i + j] += xi;
            }
        }
    }
}

void fft_free(void) {
    free(cos_tab); free(sin_tab); free(rev);
    cos_tab = sin_tab = NULL; rev = NULL; fft_n = 0;
}
