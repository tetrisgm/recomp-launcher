// launcher_boot_timing.h — opt-in wall-clock stamps (PSX_LAUNCHER_BOOT_TIMING=1).
#ifndef LAUNCHER_BOOT_TIMING_H
#define LAUNCHER_BOOT_TIMING_H
#ifdef __cplusplus
extern "C" {
#endif
void launcher_boot_timing_mark(const char *phase);
#ifdef __cplusplus
}
#endif
#endif
