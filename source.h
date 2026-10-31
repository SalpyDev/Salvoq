
#ifndef SALVOQ_SOURCE_H
#define SALVOQ_SOURCE_H
#include "capture.h"
#include "analysis.h"
typedef struct {
    capture_source cap;
    analysis an;
    char label[128];
} source;
#endif
