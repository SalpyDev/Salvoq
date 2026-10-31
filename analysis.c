
#include "analysis.h"
#include "fft.h"
#include "window.h"
#include "common.h"
#include <math.h>
#include <string.h>

void analysis_init(analysis *a, int bars, float floor_db, float ceil_db, float tilt,
                   float decay, int smooth, double peak_hold, double peak_fall) {
    memset(a, 0, sizeof *a);
    a->bars = bars;
    a->db_floor = floor_db;
    a->db_ceil = ceil_db;
    a->tilt = tilt;
    a->decay = decay;
    a->smooth_attack = smooth;
    a->peak_hold = peak_hold;
    a->peak_fall = peak_fall;
    fft_init(SALVOQ_FFT_SIZE);
    window_init(SALVOQ_FFT_SIZE);
}

static float bin_value(const analysis *a, double freq) {
    double idx = freq * SALVOQ_FFT_SIZE / SALVOQ_SAMPLE_RATE;
    size_t i0 = (size_t)floor(idx), i1 = (size_t)ceil(idx);
    if (i0 >= SALVOQ_FFT_SIZE / 2) i0 = SALVOQ_FFT_SIZE / 2 - 1;
    if (i1 >= SALVOQ_FFT_SIZE / 2) i1 = SALVOQ_FFT_SIZE / 2 - 1;
    float f = (float)(idx - floor(idx));
    return a->mag[i0] * (1.0f - f) + a->mag[i1] * f;
}

void analysis_update(analysis *a, ringbuf *ring, double now_ns) {
    float incoming[SALVOQ_RING_CAPACITY];
    size_t got = rb_read(ring, incoming, SALVOQ_RING_CAPACITY);
    if (got >= SALVOQ_HOP) {
        memmove(a->history, a->history + got, (SALVOQ_FFT_SIZE - got) * sizeof(float));
        memcpy(a->history + SALVOQ_FFT_SIZE - got, incoming, got * sizeof(float));
        window_apply(a->history, a->re, SALVOQ_FFT_SIZE);
        memset(a->im, 0, sizeof a->im);
        fft_forward(a->re, a->im, SALVOQ_FFT_SIZE);
        for (size_t k = 0; k <= SALVOQ_FFT_SIZE / 2; k++) {
            float m = sqrtf(a->re[k] * a->re[k] + a->im[k] * a->im[k]) * 4.0f / SALVOQ_FFT_SIZE;
            if (a->tilt) {
                double f = (double)k * SALVOQ_SAMPLE_RATE / SALVOQ_FFT_SIZE;
                if (f < 1.0) f = 1.0;
                m *= (float)pow(10.0, a->tilt * log2(f / 1000.0) / 20.0);
            }
            a->mag[k] = m;
        }
        double dt = (now_ns - a->last_frame_ns) / 1e9;
        if (dt <= 0) dt = 1.0 / 60.0;
        for (int i = 0; i < a->bars; i++) {
            double lo = 20.0 * pow(1000.0, (double)i / a->bars);
            double hi = 20.0 * pow(1000.0, (double)(i + 1) / a->bars);
            double center = 20.0 * pow(1000.0, ((double)i + 0.5) / a->bars);
            float maxm = 0.0f;
            int covered = 0;
            for (size_t k = 1; k <= SALVOQ_FFT_SIZE / 2; k++) {
                double f = (double)k * SALVOQ_SAMPLE_RATE / SALVOQ_FFT_SIZE;
                if (f >= lo && f < hi) { if (a->mag[k] > maxm) maxm = a->mag[k]; covered = 1; }
            }
            if (!covered) maxm = bin_value(a, center);
            double db = 20.0 * log10(maxm > 1e-9f ? maxm : 1e-9f);
            double v = (db - a->db_floor) / (a->db_ceil - a->db_floor);
            if (v < 0) v = 0;
            if (v > 1) v = 1;
            float old = a->values[i], nv = (float)v;
            float out;
            if (a->smooth_attack) out = old + (nv - old) * 0.5f;
            else out = nv > old ? nv : old;
            out = out * a->decay;
            if (out < nv) out = nv;
            if (out < 0.001f) out = 0.0f;
            a->values[i] = out;
            if (out >= a->peaks[i]) { a->peaks[i] = out; a->peak_time[i] = now_ns; }
            else if (a->peak_fall > 0 && now_ns - a->peak_time[i] > a->peak_hold * 1e9) {
                a->peaks[i] -= (float)(a->peak_fall * dt);
                if (a->peaks[i] < out) a->peaks[i] = out;
            }
        }
        a->last_frame_ns = now_ns;
    }
}
