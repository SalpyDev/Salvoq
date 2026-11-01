
#ifndef SALVOQ_CANVAS_H
#define SALVOQ_CANVAS_H
#include <stdint.h>
typedef struct { char glyph[5]; uint8_t fr, fg, fb, br, bg, bb; } cell;
typedef struct { cell *front, *back; int cols, rows; } canvas;
void canvas_init(canvas *c, int cols, int rows);
void canvas_free(canvas *c);
void canvas_clear(canvas *c);
void canvas_set(canvas *c, int x, int y, const char *glyph, uint8_t rgb[6]);
void canvas_flush(canvas *c);
#endif
