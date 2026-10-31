
#include "term.h"
#include "platform.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

static atomic_int restored;
static atomic_bool in_frame;
static unsigned long saved_in, saved_out, saved_cp;

static void do_restore(void) {
    if (atomic_exchange(&restored, 1)) return;
    int waited = 0;
    while (atomic_load(&in_frame) && waited < 100) { plat_sleep_ns(1000000); waited++; }
    plat_write("\x1b[?25h\x1b[?1049l\x1b[0m", 19);
    plat_console_set_modes(saved_in, saved_out);
    plat_set_output_cp(saved_cp);
}

void term_restore(void) { do_restore(); }

static void on_close(void) { do_restore(); }

int term_init(void) {
    if (plat_console_get_modes(&saved_in, &saved_out) != 0) return -1;
    if (plat_console_set_input_raw() != 0) return -1;
    if (plat_console_enable_vt() != 0) {
        fprintf(stderr, "salvoq: this console does not support virtual terminal sequences\n");
        return -2;
    }
    saved_cp = plat_get_output_cp();
    plat_set_output_cp(65001);
    plat_set_close_handler(on_close);
    plat_at_exit(do_restore);
    plat_write("\x1b[?1049h\x1b[?25l\x1b[2J", 17);
    return 0;
}

void term_begin_frame(void) { atomic_store(&in_frame, 1); }
void term_end_frame(void) { atomic_store(&in_frame, 0); }
