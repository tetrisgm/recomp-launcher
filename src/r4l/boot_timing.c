#include "launcher_boot_timing.h"

#include <SDL3/SDL_timer.h>
#include <stdio.h>
#include <stdlib.h>

/* SDL's clock, not timespec_get: MinGW's C runtime lacks the latter. */
void launcher_boot_timing_mark(const char *phase) {
    static int enabled = -1;
    static Uint64 t0;
    if (enabled < 0) {
        const char *e = getenv("PSX_LAUNCHER_BOOT_TIMING");
        enabled = e && *e && *e != '0';
        t0 = SDL_GetTicksNS();
    }
    if (!enabled || !phase) return;
    fprintf(stderr, "[boot-timing] %8.3f ms %s\n", (SDL_GetTicksNS() - t0) / 1e6, phase);
}
