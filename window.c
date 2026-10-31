
#include "window.h"
#include "common.h"
#include <math.h>
#include <stdlib.h>

static float *hann;
static size_t win_n;

void window_init(size_t n) {
    win_n = n;
    hann = malloc(sizeof(float) * n);
    for (size_t i = 0; i < n; i++)
        hann[i] = (float)(0.5 * (1.0 - cos(2.0 * SALVOQ_PI * (double)i / (double)(n - 1))));
}

const float *window_get(void) { return hann; }

void window_apply(const float *in, float *out, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = in[i] * hann[i];
}

void window_free(void) { free(hann); hann = NULL; win_n = 0; }
