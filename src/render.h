
#ifndef SALVOQ_RENDER_H
#define SALVOQ_RENDER_H
#include "analysis.h"
#include "canvas.h"
typedef struct { uint8_t rgb[3]; } color;
typedef struct {
    color grad[6], bass, mids, treble, peak, bg;
    int grad_count;
    int tint, mirror, gap, ascii, color_depth;
    int tint_strength, tint_height;
    int has_bg;
} theme;
void render_frame(canvas *c, analysis *a, const theme *t, const char *status);
void theme_load(theme *t, const char *name);
#endif
