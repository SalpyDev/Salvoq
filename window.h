
#ifndef SALVOQ_WINDOW_H
#define SALVOQ_WINDOW_H
#include <stddef.h>
void window_init(size_t n);
const float *window_get(void);
void window_apply(const float *in, float *out, size_t n);
void window_free(void);
#endif
