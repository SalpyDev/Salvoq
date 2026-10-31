
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "capture.h"
#include "procfind.h"
#include "platform.h"
#include "common.h"

static DWORD WINAPI process_thread(void *arg) {
    capture_source *s = arg;
    wchar_t wname[128];
    size_t n = strlen(s->name);
    mbstowcs(wname, s->name, 127);
    wname[127] = 0;
    if (n > 4 && strcmp(s->name + n - 4, ".exe") != 0) wcscat(wname, L".exe");
    DWORD pid = procfind_pick_root(wname);
    if (!pid) { fprintf(stderr, "salvoq: no running process named \"%s\"\n", s->name); exit(2); }
    fprintf(stderr, "salvoq: process loopback for %s (PID %lu)\n", s->name, (unsigned long)pid);
    fprintf(stderr, "salvoq: --app process loopback is not available in this build\n");
    exit(2);
    return 0;
}

int capture_start_process(capture_source *s, const char *app) {
    strncpy(s->name, app, sizeof s->name - 1);
    atomic_store(&s->running, 1);
    plat_thread t;
    return plat_thread_create(&t, process_thread, s);
}
