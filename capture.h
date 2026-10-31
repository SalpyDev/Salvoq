
#ifndef SALVOQ_CAPTURE_H
#define SALVOQ_CAPTURE_H
#include "ringbuf.h"
typedef struct capture_backend capture_backend;
typedef struct {
    ringbuf ring;
    _Atomic int running;
    _Atomic int ended;
    char name[128];
} capture_source;
int capture_start_wasapi(capture_source *src, const char *device);
int capture_start_test(capture_source *src);
int capture_start_process(capture_source *src, const char *app);
void capture_stop(capture_source *src);
#endif
