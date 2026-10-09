/* abi_conformance.c — compiled as C, the way psxrecomp's host code consumes
 * the launcher headers. Every macro, type and symbol the runtime (main.cpp,
 * mod_runtime.cpp, psxrecomp_codegen_host.c) uses at the pinned psxrecomp
 * must exist here, or this file fails to build. */
#include "recomp_launcher.h"
#include "launcher_profile.h"
#include "launcher_boot_timing.h"

#include <stddef.h>

#define R4L_REQUIRE(m) _Static_assert((m) || 1, #m)
#if !defined(RECOMP_LAUNCHER_HAS_INTERNAL_RESOLUTION) || !defined(RECOMP_LAUNCHER_HAS_DYNAMIC_RESOLUTION) || \
    !defined(RECOMP_LAUNCHER_HAS_RENDER_PIPELINE) || !defined(RECOMP_LAUNCHER_HAS_SCANLINES) ||          \
    !defined(RECOMP_LAUNCHER_HAS_PLAYER_ACCOUNT) || !defined(RECOMP_LAUNCHER_HAS_MULTITAP_ENABLED) ||     \
    !defined(RECOMP_LAUNCHER_HAS_MULTITAP_ANALOG) || !defined(RECOMP_LAUNCHER_HAS_HOST_RELAY) ||          \
    !defined(RECOMP_LAUNCHER_HAS_SBI_STATUS) || !defined(RECOMP_LAUNCHER_HAS_CHAT_REPORT) ||              \
    !defined(RECOMP_LAUNCHER_HAS_AUTOMATCH) || !defined(RECOMP_LAUNCHER_HAS_WORKER_MOD_COMMIT) ||         \
    !defined(RECOMP_LAUNCHER_HAS_SET_BLOCKS) || !defined(RECOMP_LAUNCHER_HAS_PREPARED_MOD_COMMIT) ||      \
    !defined(RECOMP_LAUNCHER_HAS_NETPLAY_HANDOFF) || !defined(RECOMP_LAUNCHER_HAS_LIST_SCOPE) ||          \
    !defined(RECOMP_LAUNCHER_HAS_DIRECT_ASSIST_BIND) || !defined(RECOMP_LAUNCHER_HAS_ACCOUNT) ||          \
    !defined(RECOMP_LAUNCHER_HAS_QUALITY_PRESETS) || !defined(RECOMP_LAUNCHER_HAS_PRESERVE_SDL)
#error "launcher ABI header is missing a feature macro the runtime tests"
#endif

_Static_assert(RECOMP_LAUNCHER_MAX_PLAYERS == 8, "player storage ceiling");
_Static_assert(RECOMP_LAUNCHER_MAX_ASSIST_BINDINGS == 8, "assist slots");
_Static_assert(RECOMP_LAUNCHER_NETPLAY_MAX_MEMBERS == 8, "netplay members");
_Static_assert(RECOMP_LAUNCHER_RESULT_LAUNCH == 0 && RECOMP_LAUNCHER_RESULT_QUIT == 1 &&
               RECOMP_LAUNCHER_RESULT_UNAVAILABLE == 2 && RECOMP_LAUNCHER_RESULT_RELAUNCH == 3,
               "result codes");
_Static_assert(RECOMP_MOD_OPTION_BOOLEAN == 0 && RECOMP_MOD_OPTION_CHOICE == 1 &&
               RECOMP_MOD_OPTION_INTEGER == 2, "mod option types");
_Static_assert(RECOMP_SBI_NA == 0 && RECOMP_SBI_MISSING == 1 && RECOMP_SBI_OK == 2, "sbi");

/* Address every linked symbol so a missing definition is a link error. */
typedef int (*run_fn)(const char*, RecompLauncherCSettings*, const RecompLauncherCGameInfo*, const char*,
                      const char*, char*, size_t);
int r4l_abi_symbols_present(void) {
    run_fn a = recomp_launcher_run_window;
    int (*b)(char*, size_t) = recomp_launcher_relaunch_exe;
    void (*c)(int) = recomp_launcher_set_preserve_sdl;
    void (*d)(const char*) = launcher_boot_timing_mark;
    RecompLauncherCGameInfo gi = {0};
    int ok = launcher_profile_apply("psx", &gi) == 1 && gi.has_bios && gi.pad_mode_supported;
    return ok && a && b && c && d;
}
