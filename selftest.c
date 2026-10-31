
#include "selftest.h"
#include "fft.h"
#include "window.h"
#include "common.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int fail;
static void check(int ok, const char *msg) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", msg); fail = 1; }
}

int selftest_run(void) {
    size_t n = SALVOQ_FFT_SIZE;
    float re[n], im[n], ref[n];
    fft_init(n);
    window_init(n);
    memset(re, 0, sizeof re); memset(im, 0, sizeof im);
    re[0] = 1.0f;
    fft_forward(re, im, n);
    for (size_t k = 0; k < n; k++) check(fabsf(im[k]) < 1e-4f && fabsf(re[k] - 1.0f) < 1e-4f, "impulse spectrum");
    memset(re, 0, sizeof re); memset(im, 0, sizeof im);
    for (size_t i = 0; i < n; i++) re[i] = (float)sin(2.0 * SALVOQ_PI * 8.0 * i / n);
    fft_forward(re, im, n);
    float peak = 0; size_t pk = 0;
    for (size_t k = 0; k < n; k++) if (hypotf(re[k], im[k]) > peak) { peak = hypotf(re[k], im[k]); pk = k; }
    check(pk == 8, "sine peak bin");
    for (size_t i = 0; i < n; i++) {
        re[i] = (float)rand() / RAND_MAX - 0.5f;
        im[i] = 0;
        ref[i] = re[i];
    }
    fft_forward(re, im, n);
    double maxerr = 0;
    for (size_t k = 0; k < n; k++) {
        double sr = 0, si = 0;
        for (size_t j = 0; j < n; j++) {
            double a = -2.0 * SALVOQ_PI * (double)k * j / n;
            sr += ref[j] * cos(a); si += ref[j] * sin(a);
        }
        double e = hypot(re[k] - sr, im[k] - si);
        if (e > maxerr) maxerr = e;
    }
    check(maxerr < 1e-2, "FFT matches DFT");
    const float *w = window_get();
    check(fabsf(w[0]) < 1e-6f && fabsf(w[n - 1]) < 1e-6f, "Hann endpoints");
    int sym = 1;
    for (size_t i = 0; i < n; i++) if (fabsf(w[i] - w[n - 1 - i]) > 1e-5f) sym = 0;
    check(sym, "Hann symmetry");
    fft_free(); window_free();
    printf("selftest: %s\n", fail ? "FAILED" : "ok");
    return fail;
}
