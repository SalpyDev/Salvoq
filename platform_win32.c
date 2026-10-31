
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <stdio.h>
#include "platform.h"

static LARGE_INTEGER qpf;
static HANDLE high_res_timer;

static void platform_once(void) {
    QueryPerformanceFrequency(&qpf);
    high_res_timer = CreateWaitableTimerExW(NULL, NULL,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
}

uint64_t plat_now_ns(void) {
    LARGE_INTEGER c;
    if (!qpf.QuadPart) platform_once();
    QueryPerformanceCounter(&c);
    return (uint64_t)((double)c.QuadPart * 1e9 / (double)qpf.QuadPart);
}

void plat_sleep_ns(uint64_t ns) {
    if (!high_res_timer) platform_once();
    if (high_res_timer) {
        LARGE_INTEGER due;
        due.QuadPart = -(LONGLONG)(ns / 100);
        if (SetWaitableTimer(high_res_timer, &due, 0, NULL, NULL, 0)) {
            WaitForSingleObject(high_res_timer, INFINITE);
            return;
        }
    }
    timeBeginPeriod(1);
    Sleep((DWORD)(ns / 1000000));
    timeEndPeriod(1);
}

int plat_thread_create(plat_thread *t, void (*fn)(void *), void *arg) {
    HANDLE h = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)fn, arg, 0, NULL);
    if (!h) return -1;
    *t = h;
    return 0;
}

void plat_thread_join(plat_thread t) {
    WaitForSingleObject((HANDLE)t, INFINITE);
    CloseHandle((HANDLE)t);
}

int plat_write(const char *buf, size_t n) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    size_t done = 0;
    while (done < n) {
        if (!WriteFile(h, buf + done, (DWORD)(n - done), &written, NULL) || !written) return -1;
        done += written;
    }
    return 0;
}

int plat_size(int *cols, int *rows) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(h, &info)) return -1;
    *cols = info.srWindow.Right - info.srWindow.Left + 1;
    *rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    return 0;
}

int plat_console_enable_vt(void) {
    DWORD mode;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleMode(h, &mode)) return -1;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    return SetConsoleMode(h, mode) ? 0 : -1;
}

int plat_console_get_modes(unsigned long *in, unsigned long *out) {
    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), (LPDWORD)in)) return -1;
    if (!GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), (LPDWORD)out)) return -1;
    return 0;
}

int plat_console_set_modes(unsigned long in, unsigned long out) {
    if (!SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), (DWORD)in)) return -1;
    if (!SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), (DWORD)out)) return -1;
    return 0;
}

int plat_console_set_input_raw(void) {
    DWORD mode;
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(h, &mode)) return -1;
    mode &= ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    return SetConsoleMode(h, mode) ? 0 : -1;
}

unsigned plat_get_output_cp(void) { return GetConsoleOutputCP(); }
int plat_set_output_cp(unsigned cp) { return SetConsoleOutputCP(cp) ? 0 : -1; }

int plat_read_keys(unsigned char *buf, int max) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD count = 0, read = 0, n = 0;
    INPUT_RECORD rec[64];
    if (!GetNumberOfConsoleInputEvents(h, &count) || !count) return 0;
    if (count > 64) count = 64;
    if (!ReadConsoleInputW(h, rec, count, &read)) return 0;
    for (DWORD i = 0; i < read && n < (DWORD)max; i++) {
        if (rec[i].EventType != KEY_EVENT || !rec[i].Event.KeyEvent.bKeyDown) continue;
        WCHAR c = rec[i].Event.KeyEvent.uChar.AsciiChar;
        if (c >= 1 && c <= 255) buf[n++] = (unsigned char)c;
    }
    return (int)n;
}

static void (*close_fn)(void);
static BOOL WINAPI ctrl_handler(DWORD type) {
    if (type == CTRL_CLOSE_EVENT || type == CTRL_LOGOFF_EVENT || type == CTRL_SHUTDOWN_EVENT) {
        if (close_fn) close_fn();
        return TRUE;
    }
    return FALSE;
}

int plat_set_close_handler(void (*fn)(void)) { close_fn = fn; return SetConsoleCtrlHandler(ctrl_handler, TRUE) ? 0 : -1; }
int plat_at_exit(void (*fn)(void)) { return atexit(fn) == 0 ? 0 : -1; }
