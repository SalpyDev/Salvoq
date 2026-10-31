
#ifndef SALVOQ_PLATFORM_H
#define SALVOQ_PLATFORM_H
#include <stddef.h>
#include <stdint.h>
typedef void *plat_thread;
uint64_t plat_now_ns(void);
void plat_sleep_ns(uint64_t ns);
int plat_thread_create(plat_thread *t, void (*fn)(void *), void *arg);
void plat_thread_join(plat_thread t);
int plat_write(const char *buf, size_t n);
int plat_size(int *cols, int *rows);
int plat_console_enable_vt(void);
int plat_console_get_modes(unsigned long *in, unsigned long *out);
int plat_console_set_modes(unsigned long in, unsigned long out);
int plat_console_set_input_raw(void);
unsigned plat_get_output_cp(void);
int plat_set_output_cp(unsigned cp);
int plat_read_keys(unsigned char *buf, int max);
int plat_set_close_handler(void (*fn)(void));
int plat_at_exit(void (*fn)(void));
#endif
