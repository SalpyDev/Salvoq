
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "capture.h"
#include "platform.h"
#include "common.h"

static DWORD WINAPI test_thread(void *arg) {
    capture_source *s = arg;
    uint64_t t0 = plat_now_ns(), pushed = 0;
    float block[512];
    size_t i = 0;
    while (atomic_load(&s->running)) {
        for (size_t k = 0; k < 512; k++, i++) {
            double t = (double)i / SALVOQ_SAMPLE_RATE;
            double f = 20.0 * pow(1000.0, t / 8.0);
            if (f > 20000.0) f = 20.0 + fmod(t * 500.0, 19980.0);
            block[k] = (float)(0.25 * sin(2.0 * SALVOQ_PI * f * t) +
                               0.10 * sin(2.0 * SALVOQ_PI * 60.0 * t) +
                               0.08 * sin(2.0 * SALVOQ_PI * 1000.0 * t) +
                               0.06 * sin(2.0 * SALVOQ_PI * 8000.0 * t));
        }
        rb_write(&s->ring, block, 512);
        pushed += 512;
        uint64_t due = ((plat_now_ns() - t0) * SALVOQ_SAMPLE_RATE / 1000000000ULL) - pushed;
        if (due > SALVOQ_SAMPLE_RATE) { t0 = plat_now_ns(); pushed = 0; }
        else if (due < 512) plat_sleep_ns((512 - due) * 1000000000ULL / SALVOQ_SAMPLE_RATE);
    }
    return 0;
}

int capture_start_test(capture_source *s) {
    strcpy(s->name, "TEST TONE");
    atomic_store(&s->running, 1);
    plat_thread t;
    return plat_thread_create(&t, test_thread, s);
}
