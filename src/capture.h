
#ifndef SALVOQ_CAPTURE_H
#define SALVOQ_CAPTURE_H
#include "ringbuf.h"
#include "platform.h"
typedef struct capture_backend capture_backend;
typedef struct {
    ringbuf ring;
    _Atomic int running;
    _Atomic int ended;
    char name[128];
    plat_thread thread;
} capture_source;
int capture_start_wasapi(capture_source *src, const char *device);
int capture_start_test(capture_source *src);
int capture_start_process(capture_source *src, const char *app);

static inline void capture_stop(capture_source *src) {
    if (!src->thread) return;
    atomic_store(&src->running, 0);
    plat_thread_join(src->thread);
    src->thread = NULL;
}
#endif
