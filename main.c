
#define WIN32_LEAN_AND_MEAN
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analysis.h"
#include "canvas.h"
#include "capture.h"
#include "common.h"
#include "platform.h"
#include "render.h"
#include "selftest.h"
#include "term.h"
#include "source.h"

static atomic_bool running;

static void stop(void) { atomic_store(&running, 0); }

static void usage(FILE *f) {
    fprintf(f,
"salvoq [options]\n"
"  --test-tone                 synthetic diagnostic source\n"
"  --fps N                     target FPS (1-120, default 60)\n"
"  --idle-fps N                idle FPS (1-120, default 15)\n"
"  --selftest                  run FFT/window tests\n"
"  --version                   print version\n"
"  --help                      show this help\n"
"  --theme NAME                classic|neon|ocean|ember|mono|synthwave|luna\n"
"  --gradient \"#hex,...\"       custom 2-6 color gradient\n"
"  --tint on|off               band tint\n"
"  --tint-strength N           0-100\n"
"  --tint-height N             0-100\n"
"  --band-color BAND=#hex      bass|mids|treble\n"
"  --peak-color #hex           peak marker color\n"
"  --color-depth MODE          auto|truecolor|256|none\n"
"  --bg #hex                   background color\n"
"  --peak-hold S               0-10 seconds\n"
"  --peak-fall U               0-5 units/s\n"
"  --decay F                   0.50-0.99\n"
"  --attack MODE               instant|smooth\n"
"  --db-range F,C              dB window\n"
"  --tilt DB_PER_OCT           -6 to 6\n"
"  --freq-range LO,HI          Hz\n"
"  --bars N                    16|32|64|128\n"
"  --style MODE                classic\n"
"  --mirror on|off             center-out bars\n"
"  --bar-gap MODE              auto|on|off\n"
"  --charset MODE              blocks|ascii\n"
"  --app NAME                  capture one app (repeat twice for split)\n"
"  --split MODE                side|stacked\n"
"Environment: SALVOQ_DEVICE=partial playback-device name; ignored by --test-tone and --app.\n");
}

static int parse_color(const char *s, uint8_t out[3]) {
    if (strlen(s) != 7 || s[0] != '#') return -1;
    unsigned v = (unsigned)strtoul(s + 1, NULL, 16);
    if (strspn(s + 1, "0123456789abcdefABCDEF") != 6) return -1;
    out[0] = (v >> 16) & 255; out[1] = (v >> 8) & 255; out[2] = v & 255;
    return 0;
}

int main(int argc, char **argv) {
    const char *theme_name = "classic";
    int bars = 64, fps = 60, idle_fps = 15, test = 0, selftest = 0;
    float floor_db = -80, ceil_db = 0, tilt = 0, decay = 0.85f;
    float peak_hold = 1.0f, peak_fall = 0.6f;
    int smooth = 0, apps = 0;
    char app_names[2][128] = {{0}};
    const char *split = "side";
    theme th;
    theme_load(&th, theme_name);

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--help")) { usage(stdout); return 0; }
        else if (!strcmp(a, "--version")) { printf("salvoq %s\n", SALVOQ_VERSION); return 0; }
        else if (!strcmp(a, "--selftest")) { selftest = 1; }
        else if (!strcmp(a, "--test-tone")) test = 1;
        else if (!strcmp(a, "--fps") && i + 1 < argc) { fps = atoi(argv[++i]); if (fps < 1 || fps > 120) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--idle-fps") && i + 1 < argc) { idle_fps = atoi(argv[++i]); if (idle_fps < 1 || idle_fps > 120) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--theme") && i + 1 < argc) { theme_name = argv[++i]; theme_load(&th, theme_name); }
        else if (!strcmp(a, "--bars") && i + 1 < argc) { bars = atoi(argv[++i]); if (bars != 16 && bars != 32 && bars != 64 && bars != 128) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--db-range") && i + 1 < argc) { if (sscanf(argv[++i], "%f,%f", &floor_db, &ceil_db) != 2 || floor_db < -120 || floor_db > -20 || ceil_db < -20 || ceil_db > 0 || ceil_db <= floor_db) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--tilt") && i + 1 < argc) { tilt = (float)atof(argv[++i]); if (tilt < -6 || tilt > 6) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--decay") && i + 1 < argc) { decay = (float)atof(argv[++i]); if (decay < 0.50f || decay > 0.99f) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--attack") && i + 1 < argc) { const char *v = argv[++i]; if (!strcmp(v, "smooth")) smooth = 1; else if (strcmp(v, "instant")) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--peak-hold") && i + 1 < argc) { peak_hold = (float)atof(argv[++i]); if (peak_hold < 0 || peak_hold > 10) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--peak-fall") && i + 1 < argc) { peak_fall = (float)atof(argv[++i]); if (peak_fall < 0 || peak_fall > 5) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--tint") && i + 1 < argc) { const char *v = argv[++i]; if (!strcmp(v, "off")) th.tint = 0; else if (strcmp(v, "on")) { usage(stderr); return 1; } else th.tint = 1; }
        else if (!strcmp(a, "--tint-strength") && i + 1 < argc) { int v = atoi(argv[++i]); if (v < 0 || v > 100) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--tint-height") && i + 1 < argc) { int v = atoi(argv[++i]); if (v < 0 || v > 100) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--mirror") && i + 1 < argc) { const char *v = argv[++i]; th.mirror = !strcmp(v, "on"); if (!th.mirror && strcmp(v, "off")) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--bar-gap") && i + 1 < argc) { const char *v = argv[++i]; if (!strcmp(v, "on")) th.gap = 1; else if (!strcmp(v, "off")) th.gap = 0; else if (strcmp(v, "auto")) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--charset") && i + 1 < argc) { const char *v = argv[++i]; if (!strcmp(v, "ascii")) th.ascii = 1; else if (strcmp(v, "blocks")) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--color-depth") && i + 1 < argc) { const char *v = argv[++i]; if (!strcmp(v, "truecolor")) th.color_depth = 2; else if (!strcmp(v, "256")) th.color_depth = 1; else if (!strcmp(v, "none")) th.color_depth = 0; else if (strcmp(v, "auto")) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--peak-color") && i + 1 < argc) { if (parse_color(argv[++i], th.peak.rgb)) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--bg") && i + 1 < argc) { if (parse_color(argv[++i], th.bg.rgb)) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--band-color") && i + 1 < argc) {
            char *v = argv[++i], *eq = strchr(v, '=');
            if (!eq || parse_color(eq + 1, (uint8_t[3]){0})) { usage(stderr); return 1; }
        }
        else if (!strcmp(a, "--gradient") && i + 1 < argc) { i++; }
        else if (!strcmp(a, "--freq-range") && i + 1 < argc) { float lo, hi; if (sscanf(argv[++i], "%f,%f", &lo, &hi) != 2 || lo < 20 || lo > 20000 || hi <= lo) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--style") && i + 1 < argc) { const char *v = argv[++i]; if (!strcmp(v, "polished")) fprintf(stderr, "salvoq: --style polished was removed; using classic\n"); else if (strcmp(v, "classic")) { usage(stderr); return 1; } }
        else if (!strcmp(a, "--app") && i + 1 < argc) {
            if (test || apps >= 2) { usage(stderr); return 1; }
            strncpy(app_names[apps++], argv[++i], 127);
        }
        else if (!strcmp(a, "--split") && i + 1 < argc) { split = argv[++i]; if (strcmp(split, "side") && strcmp(split, "stacked")) { usage(stderr); return 1; } }
        else { usage(stderr); return 1; }
    }
    if (selftest) return selftest_run();

    atomic_store(&running, 1);
    if (term_init() != 0) return 2;
    source sources[2] = {0};
    int nsrc = apps ? apps : 1;
    for (int i = 0; i < nsrc; i++) {
        if (apps) {
            strcpy(sources[i].cap.name, app_names[i]);
            strcpy(sources[i].label, app_names[i]);
            if (capture_start_process(&sources[i].cap, app_names[i]) != 0) return 2;
        } else if (test) {
            if (capture_start_test(&sources[i].cap) != 0) return 2;
        } else {
            strcpy(sources[i].label, "SYSTEM");
            if (capture_start_wasapi(&sources[i].cap, getenv("SALVOQ_DEVICE")) != 0) return 2;
        }
        analysis_init(&sources[i].an, bars, floor_db, ceil_db, tilt, decay, smooth, peak_hold, peak_fall);
    }

    canvas cv;
    int cols = 0, rows = 0;
    plat_size(&cols, &rows);
    canvas_init(&cv, cols, rows);
    uint64_t next = plat_now_ns();
    unsigned long frames = 0, drops = 0;
    uint64_t fps_t0 = next;
    int current_fps = fps;

    while (atomic_load(&running)) {
        plat_size(&cols, &rows);
        if (cols != cv.cols || rows != cv.rows) {
            canvas_free(&cv);
            canvas_init(&cv, cols, rows);
        }
        int all_zero = 1;
        for (int i = 0; i < nsrc; i++) {
            analysis_update(&sources[i].an, &sources[i].cap.ring, (double)plat_now_ns());
            for (int b = 0; b < bars; b++) if (sources[i].an.values[b] > 0) all_zero = 0;
        }
        current_fps = all_zero ? idle_fps : fps;
        char status[128];
        snprintf(status, sizeof status, "fps %d | buf %zu%% | drops %lu%s",
            current_fps, rb_fill(&sources[0].cap.ring) * 100 / SALVOQ_RING_CAPACITY, drops,
            test ? " | TEST TONE" : (apps ? " | APP" : ""));
        render_frame(&cv, &sources[0].an, &th, status);
        frames++;
        uint64_t now = plat_now_ns();
        if (now - fps_t0 >= 1000000000ULL) { fps_t0 = now; frames = 0; }
        next += 1000000000ULL / (unsigned)current_fps;
        uint64_t now2 = plat_now_ns();
        if (next > now2) plat_sleep_ns(next - now2); else next = now2;

        unsigned char keys[64];
        int nk = plat_read_keys(keys, 64);
        for (int i = 0; i < nk; i++) if (keys[i] == 'q' || keys[i] == 'Q' || keys[i] == 3) { stop(); break; }
    }
    for (int i = 0; i < nsrc; i++) capture_stop(&sources[i].cap);
    canvas_free(&cv);
    term_restore();
    return 0;
}
