// launcher_profile.h — per-console GameInfo defaults. The runtime calls
// launcher_profile_apply("psx", gi) before filling title fields.
#ifndef LAUNCHER_PROFILE_H
#define LAUNCHER_PROFILE_H

#include "recomp_launcher.h"

#ifndef RECOMP_UI_PSX_HAS_REWIND
#define RECOMP_UI_PSX_HAS_REWIND 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline int r4l_ieq(const char* a, const char* b) {
    if (!a || !b) return 0;
    for (; *a && *b; ++a, ++b) {
        char x = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
        char y = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
        if (x != y) return 0;
    }
    return *a == *b;
}

static inline int launcher_profile_apply(const char* name, RecompLauncherCGameInfo* gi) {
    if (!gi) return 0;
    gi->rom_noun = "ROM";
    gi->theme = NULL;
    if (!(r4l_ieq(name, "psx") || r4l_ieq(name, "ps1") || r4l_ieq(name, "playstation"))) return 0;
    gi->theme = "psx";
    gi->platform = "PLAYSTATION";
    gi->rom_noun = "Disc";
    gi->pad_mode_supported = 1;
    gi->pad_mode_selectable = 1;
    gi->widescreen_supported = 0; /* display enhancements are mod-owned on PSX */
    gi->aspect_mask = 0;
    gi->has_window_size = 1;
    gi->has_renderer = 1;
    gi->has_supersampling = 1;
    gi->has_antialiasing = 1;
    gi->has_texture_filter = 1;
    gi->has_screen_kind = 1;
    gi->has_fmv_filter = 1;
    gi->has_frame_interp = 0;
    gi->has_spu_hq = 1;
    gi->has_skip_fmv = 0;
    gi->has_scanlines = 1;
    gi->has_turbo_loads = 1;
    gi->has_bios = 1;
    gi->has_deadzone_pct = 1;
    gi->has_rewind_depth = RECOMP_UI_PSX_HAS_REWIND ? 1 : 0;
    gi->has_vsync = 1;
    return 1;
}

#ifdef __cplusplus
}
#endif
#endif
