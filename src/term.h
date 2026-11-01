
#ifndef SALVOQ_TERM_H
#define SALVOQ_TERM_H
#include <stdint.h>
int term_init(void);
void term_restore(void);
void term_begin_frame(void);
void term_end_frame(void);
#endif
