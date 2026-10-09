// session.h — launcher-side state for one run_window call.
//
// Holds the host's Settings (edited in place, the ABI's in/out struct), the
// read-only GameInfo, the bind files and the disc picks. Everything the UI
// changes goes through here so it can be unit-tested without a window.
#pragma once

#include "binds.h"
#include "quality.h"
#include "recomp_launcher.h"

#include <string>
#include <vector>

namespace r4l {

enum class Outcome { None, Launch, Quit, Relaunch };

struct DiscPick {
    std::string path;
    RecompLauncherCDiscVerify verify{};
    bool verified = false;
};

struct Session {
    RecompLauncherCSettings* io = nullptr;
    const RecompLauncherCGameInfo* game = nullptr;
    std::string assets_dir;
    std::string exe_dir;

    std::vector<DiscPick> discs;  // one per GameInfo disc (or one)
    std::string bios_path;
    RecompLauncherCBiosVerify bios_verify{};
    bool bios_verified = false;

    KeyboardBinds keys;
    PadBinds pads;
    std::vector<std::pair<std::string, std::string>> hotkeys;
    bool binds_dirty = false;

    QualityTracker quality;
    Outcome outcome = Outcome::None;
    std::string relaunch_exe;
    std::string status;  // one-line toast

    void begin(RecompLauncherCSettings* io, const RecompLauncherCGameInfo* game,
               const char* assets_dir, const char* initial_rom);

    // Paths (GameInfo-provided, else exe-relative defaults).
    std::string keybinds_path() const;
    std::string input_path() const;
    std::string config_ini_path() const;
    std::string launcher_prefs_path() const;

    // Disc / BIOS.
    void set_disc(int index, const std::string& path);
    void set_bios(const std::string& path);
    bool media_ready() const;         // disc chosen (and BIOS ok when the title needs one)
    bool needs_setup() const;         // first-run wizard should show
    const std::string& primary_disc() const;

    // Persist picks: sidecars (unless host_persists_paths) + persist hooks.
    bool persist_media(std::string* err);

    // Commit everything that is file-backed (binds); Settings stay in *io.
    bool commit_files(std::string* err);

    // Called each frame after UI edits.
    void tick();
};

// Sidecars: rom.cfg (first disc), disc.cfg (one per line), bios.cfg.
bool write_sidecars(const std::vector<std::string>& dirs, const std::vector<std::string>& discs,
                    const std::string& bios);
std::vector<std::string> read_disc_cfg(const std::string& dir);
std::string read_rom_cache(const std::string& path);  // GameInfo.rom_cache_path

// launcher-window.ini (logical_width/height).
bool load_window_size(const std::string& path, int* w, int* h);
bool save_window_size(const std::string& path, int w, int h);

// RECOMP_NETPLAY_LAUNCH handoff. Returns RECOMP_LAUNCHER_RESULT_* or -1 when
// the env var is absent (open the window normally). Writes <record>.status.
int run_netplay_handoff(RecompLauncherCSettings* io, const RecompLauncherCGameInfo* game,
                        const char* initial_rom, char* out_rom, size_t out_len);

std::string dir_of(const std::string& path);
std::string join_path(const std::string& dir, const std::string& name);
std::string current_exe_dir();
// Hosts pass either .../assets or the exe dir; find the folder holding skins/ and fonts/.
std::string resolve_assets_dir(const char* given);

}  // namespace r4l
