
#ifndef SALVOQ_RENDER_H
#define SALVOQ_RENDER_H
#include "analysis.h"
#include "canvas.h"
typedef struct { uint8_t rgb[3]; } color;
typedef struct {
    color grad[3], bass, mids, treble, peak, bg;
    int tint, mirror, gap, ascii, color_depth;
} theme;
void render_frame(canvas *c, analysis *a, const theme *t, const char *status);
void theme_load(theme *t, const char *name);
#endif
