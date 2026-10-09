#include "launcher_boot_timing.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void launcher_boot_timing_mark(const char *phase) {
    static int enabled = -1;
    static struct timespec t0;
    struct timespec now;
    if (enabled < 0) {
        const char *e = getenv("PSX_LAUNCHER_BOOT_TIMING");
        enabled = e && *e && *e != '0';
        timespec_get(&t0, TIME_UTC);
    }
    if (!enabled || !phase) return;
    timespec_get(&now, TIME_UTC);
    fprintf(stderr, "[boot-timing] %8.3f ms %s\n",
            (now.tv_sec - t0.tv_sec) * 1e3 + (now.tv_nsec - t0.tv_nsec) / 1e6, phase);
}
