
#include "render.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void hex(const char *s, color *c) {
    unsigned v = (unsigned)strtoul(s + 1, NULL, 16);
    c->rgb[0] = (v >> 16) & 255; c->rgb[1] = (v >> 8) & 255; c->rgb[2] = v & 255;
}

void theme_load(theme *t, const char *name) {
    memset(t, 0, sizeof *t);
    t->tint = 1; t->gap = -1; t->color_depth = -1;
    t->tint_strength = 60; t->tint_height = 25; t->grad_count = 3;
    if (!strcmp(name, "neon")) { hex("#00FFA3", &t->grad[0]); hex("#FFF200", &t->grad[1]); hex("#FF00A8", &t->grad[2]); hex("#B600FF", &t->bass); hex("#00F0FF", &t->mids); hex("#3D5AFE", &t->treble); }
    else if (!strcmp(name, "ocean")) { hex("#023E8A", &t->grad[0]); hex("#00B4D8", &t->grad[1]); hex("#CAF0F8", &t->grad[2]); hex("#0077B6", &t->bass); hex("#48CAE4", &t->mids); hex("#90E0EF", &t->treble); }
    else if (!strcmp(name, "ember")) { hex("#7F1D1D", &t->grad[0]); hex("#F97316", &t->grad[1]); hex("#FDE68A", &t->grad[2]); hex("#DC2626", &t->bass); hex("#F97316", &t->mids); hex("#FACC15", &t->treble); }
    else if (!strcmp(name, "mono")) { hex("#505050", &t->grad[0]); hex("#A0A0A0", &t->grad[1]); hex("#FFFFFF", &t->grad[2]); hex("#808080", &t->bass); hex("#808080", &t->mids); hex("#808080", &t->treble); }
    else if (!strcmp(name, "synthwave")) { hex("#2D1B69", &t->grad[0]); hex("#FF2E97", &t->grad[1]); hex("#FFD319", &t->grad[2]); hex("#FF2E97", &t->bass); hex("#B537F2", &t->mids); hex("#00D9FF", &t->treble); }
    else if (!strcmp(name, "luna")) { hex("#5C5C5C", &t->grad[0]); hex("#BDBDBD", &t->grad[1]); hex("#FFFFFF", &t->grad[2]); hex("#FFFFFF", &t->bass); hex("#FFFFFF", &t->mids); hex("#FFFFFF", &t->treble); t->tint = 0; }
    else { hex("#00E676", &t->grad[0]); hex("#FFEA00", &t->grad[1]); hex("#FF1744", &t->grad[2]); hex("#D500F9", &t->bass); hex("#00E5FF", &t->mids); hex("#2979FF", &t->treble); }
    hex("#FFFFFF", &t->peak);
}

static void blend(color a, color b, float w, color *out) {
    for (int i = 0; i < 3; i++)
        out->rgb[i] = (uint8_t)(a.rgb[i] + (b.rgb[i] - a.rgb[i]) * w + 0.5f);
}

static void gradient_color(const theme *t, float fraction, color *out) {
    if (fraction <= 0.0f) { *out = t->grad[0]; return; }
    if (fraction >= 1.0f) { *out = t->grad[t->grad_count - 1]; return; }
    float scaled = fraction * (float)(t->grad_count - 1);
    int index = (int)scaled;
    blend(t->grad[index], t->grad[index + 1], scaled - (float)index, out);
}

static const char *blocks[8] = {" ", "\xe2\x96\x81", "\xe2\x96\x82", "\xe2\x96\x83", "\xe2\x96\x84", "\xe2\x96\x85", "\xe2\x96\x86", "\xe2\x96\x87"};

static float visual_value(const analysis *a, int visual_index, int visual_bars) {
    int first = visual_index * a->bars / visual_bars;
    int last = (visual_index + 1) * a->bars / visual_bars;
    float value = 0.0f;
    for (int i = first; i < last; i++)
        if (a->values[i] > value) value = a->values[i];
    return value;
}

void render_frame(canvas *c, analysis *a, const theme *t, const char *status) {
    canvas_clear(c);
    canvas_set_color_depth(c, t->color_depth);
    if (t->has_bg) {
        uint8_t background[6] = {0, 0, 0, t->bg.rgb[0], t->bg.rgb[1], t->bg.rgb[2]};
        for (int y = 0; y < c->rows; y++)
            for (int x = 0; x < c->cols; x++)
                canvas_set(c, x, y, " ", background);
    }
    int meter_rows = c->rows - 2;
    if (meter_rows < 1 || c->cols < 20) {
        canvas_set(c, 0, 0, "terminal too small", (uint8_t[6]){255,255,255,0,0,0});
        canvas_flush(c);
        return;
    }
    const int visual_bars = a->bars < 26 ? a->bars : 26;
    int gap = (t->gap == 0) ? 0 : 1;
    int unit = (c->cols + gap) / visual_bars;
    int bar_w = unit - gap;
    if (bar_w < 1) bar_w = 1;
    int left = (c->cols - (visual_bars * unit - gap)) / 2;
    if (left < 0) left = 0;
    for (int i = 0; i < visual_bars; i++) {
        float v = visual_value(a, i, visual_bars);
        int x0 = left + i * unit;
        for (int r = 0; r < meter_rows; r++) {
            int center = (meter_rows - 1) / 2;
            int distance = t->mirror ? abs(r - center) : meter_rows - 1 - r;
            int span = t->mirror ? (center > meter_rows - 1 - center ? center : meter_rows - 1 - center) : meter_rows - 1;
            float th = t->mirror ? (float)distance / (span > 0 ? span : 1) : (float)r / (meter_rows - 1);
            float level = (float)distance / meter_rows;
            if (v <= level) continue;
            color base;
            gradient_color(t, th, &base);
            color out = base;
            if (t->tint && t->tint_height > 0) {
                color tint = th < 0.125f ? t->bass : (th < 0.375f ? t->mids : t->treble);
                float height = (float)t->tint_height / 100.0f;
                float w = (float)t->tint_strength / 100.0f * (1.0f - th / height);
                if (w > 0) blend(base, tint, w, &out);
            }
            for (int x = 0; x < bar_w; x++)
                canvas_set(c, x0 + x, r, t->ascii ? "#" : blocks[7], (uint8_t[6]){out.rgb[0], out.rgb[1], out.rgb[2], 0,0,0});
        }
    }
    const char *labels[3] = {"BASS", "MIDS", "TREBLE"};
    int starts[3] = {0, visual_bars / 3, (visual_bars * 2) / 3};
    for (int b = 0; b < 3; b++) {
        int end = b == 2 ? visual_bars : starts[b + 1];
        int cx = left + ((starts[b] + end) / 2) * unit;
        for (int k = 0; labels[b][k]; k++)
            canvas_set(c, cx + k - 2, meter_rows, (char[2]){labels[b][k], 0}, (uint8_t[6]){150,150,150,0,0,0});
    }
    for (int k = 0; status[k] && k < c->cols; k++)
        canvas_set(c, k, meter_rows + 1, (char[2]){status[k], 0}, (uint8_t[6]){170,170,170,0,0,0});
    canvas_flush(c);
}
