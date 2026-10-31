
#ifndef SALVOQ_ANALYSIS_H
#define SALVOQ_ANALYSIS_H
#include "ringbuf.h"
typedef struct {
    float history[SALVOQ_FFT_SIZE];
    float re[SALVOQ_FFT_SIZE], im[SALVOQ_FFT_SIZE], mag[SALVOQ_FFT_SIZE / 2 + 1];
    float values[128], peaks[128];
    double peak_time[128];
    int bars;
    float db_floor, db_ceil, tilt, decay;
    int smooth_attack;
    double peak_hold, peak_fall;
    double last_frame_ns;
} analysis;
void analysis_init(analysis *a, int bars, float floor_db, float ceil_db, float tilt,
                   float decay, int smooth, double peak_hold, double peak_fall);
void analysis_update(analysis *a, ringbuf *ring, double now_ns);
#endif
