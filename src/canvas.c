
#include "canvas.h"
#include "platform.h"
#include "term.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void canvas_init(canvas *c, int cols, int rows) {
    c->cols = cols; c->rows = rows;
    c->color_depth = -1;
    c->front = calloc((size_t)cols * rows, sizeof(cell));
    c->back = calloc((size_t)cols * rows, sizeof(cell));
    strcpy(c->front[0].glyph, " ");
    strcpy(c->back[0].glyph, " ");
}

void canvas_set_color_depth(canvas *c, int color_depth) { c->color_depth = color_depth; }

void canvas_free(canvas *c) { free(c->front); free(c->back); c->front = c->back = NULL; }
void canvas_clear(canvas *c) {
    for (int i = 0; i < c->cols * c->rows; i++) {
        strcpy(c->back[i].glyph, " ");
        c->back[i].fr = c->back[i].fg = c->back[i].fb = 0;
        c->back[i].br = c->back[i].bg = c->back[i].bb = 0;
    }
}

void canvas_set(canvas *c, int x, int y, const char *glyph, uint8_t rgb[6]) {
    if (x < 0 || y < 0 || x >= c->cols || y >= c->rows) return;
    cell *cl = &c->back[y * c->cols + x];
    strncpy(cl->glyph, glyph, 4);
    cl->glyph[4] = 0;
    cl->fr = rgb[0]; cl->fg = rgb[1]; cl->fb = rgb[2];
    cl->br = rgb[3]; cl->bg = rgb[4]; cl->bb = rgb[5];
}

void canvas_flush(canvas *c) {
    static char out[1 << 20];
    size_t n = 0;
    n += (size_t)snprintf(out + n, sizeof out - n, "\x1b[?2026h");
    int last_fg[3] = {-1, -1, -1}, last_bg[3] = {-1, -1, -1};
    int depth = c->color_depth;
    if (depth < 0) {
        const char *wt = getenv("WT_SESSION");
        const char *color_term = getenv("COLORTERM");
        depth = wt || (color_term && (!strcmp(color_term, "truecolor") || !strcmp(color_term, "24bit"))) ? 2 : 1;
    }
    for (int y = 0; y < c->rows; y++) {
        for (int x = 0; x < c->cols; x++) {
            cell *a = &c->front[y * c->cols + x], *b = &c->back[y * c->cols + x];
            if (!strcmp(a->glyph, b->glyph) && !memcmp(a, b, sizeof(cell))) continue;
            n += (size_t)snprintf(out + n, sizeof out - n, "\x1b[%d;%dH", y + 1, x + 1);
            if (depth > 0 && memcmp(last_fg, &b->fr, 3)) {
                if (depth == 2) n += (size_t)snprintf(out + n, sizeof out - n, "\x1b[38;2;%d;%d;%dm", b->fr, b->fg, b->fb);
                else {
                    int r = (b->fr * 5 + 127) / 255, g = (b->fg * 5 + 127) / 255, bl = (b->fb * 5 + 127) / 255;
                    n += (size_t)snprintf(out + n, sizeof out - n, "\x1b[38;5;%dm", 16 + 36 * r + 6 * g + bl);
                }
                memcpy(last_fg, &b->fr, 3);
            }
            if (depth > 0 && memcmp(last_bg, &b->br, 3)) {
                if (depth == 2) n += (size_t)snprintf(out + n, sizeof out - n, "\x1b[48;2;%d;%d;%dm", b->br, b->bg, b->bb);
                else {
                    int r = (b->br * 5 + 127) / 255, g = (b->bg * 5 + 127) / 255, bl = (b->bb * 5 + 127) / 255;
                    n += (size_t)snprintf(out + n, sizeof out - n, "\x1b[48;5;%dm", 16 + 36 * r + 6 * g + bl);
                }
                memcpy(last_bg, &b->br, 3);
            }
            n += (size_t)snprintf(out + n, sizeof out - n, "%s", b->glyph);
        }
    }
    n += (size_t)snprintf(out + n, sizeof out - n, "\x1b[?2026l");
    term_begin_frame();
    plat_write(out, n);
    term_end_frame();
    cell *t = c->front; c->front = c->back; c->back = t;
}
