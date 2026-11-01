
#ifndef SALVOQ_RINGBUF_H
#define SALVOQ_RINGBUF_H
#include <stdatomic.h>
#include <stddef.h>
#include "common.h"
typedef struct {
    float data[SALVOQ_RING_CAPACITY];
    atomic_size_t head, tail;
    _Atomic unsigned long overflow;
} ringbuf;

static inline size_t rb_fill(const ringbuf *r) {
    size_t head = atomic_load_explicit(&r->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&r->tail, memory_order_acquire);
    return head - tail;
}

static inline size_t rb_write(ringbuf *r, const float *src, size_t n) {
    size_t head = atomic_load_explicit(&r->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&r->tail, memory_order_acquire);
    size_t free_space = SALVOQ_RING_CAPACITY - (head - tail);
    size_t accepted = n < free_space ? n : free_space;
    for (size_t i = 0; i < accepted; i++)
        r->data[(head + i) & (SALVOQ_RING_CAPACITY - 1u)] = src[i];
    atomic_store_explicit(&r->head, head + accepted, memory_order_release);
    if (accepted != n)
        atomic_fetch_add_explicit(&r->overflow, (unsigned long)(n - accepted), memory_order_relaxed);
    return accepted;
}

static inline size_t rb_read(ringbuf *r, float *dst, size_t n) {
    size_t tail = atomic_load_explicit(&r->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&r->head, memory_order_acquire);
    size_t available = head - tail;
    size_t count = n < available ? n : available;
    for (size_t i = 0; i < count; i++)
        dst[i] = r->data[(tail + i) & (SALVOQ_RING_CAPACITY - 1u)];
    atomic_store_explicit(&r->tail, tail + count, memory_order_release);
    return count;
}
#endif
