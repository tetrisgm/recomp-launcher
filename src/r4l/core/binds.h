// binds.h — PlayStation bind files shared with the psxrecomp runtime.
//
//  keybinds.ini  [player1..player5]  <input> = <Scancode>[, <AltScancode>]
//                (SDL scancode names, "None" = unbound; psx_keybinds.c)
//  input.ini     [controller] enabled/device/deadzone,
//                [mapping] and [mapping.<guid>]  <input> = <src>[, <src>]
//                (SDL gamepad names: a, b, dpup, leftx-, lefttrigger, ...)
//  config.ini    [KeyMap] host shortcuts (Rewind, Turbo, ...; host_keymap.c)
//
// Strings only: the UI converts to and from SDL values.
#pragma once

#include <array>
#include <string>
#include <vector>

namespace r4l {

constexpr int kPsxInputCount = 24;
constexpr int kPsxKeyboardPlayers = 5;

struct PsxInput {
    const char* key;    // ini key
    const char* label;  // shown to the player
};
extern const PsxInput kPsxInputs[kPsxInputCount];

struct KeyBind {
    std::string primary;  // SDL scancode name, "" = unbound
    std::string alt;
};

struct KeyboardBinds {
    std::array<std::array<KeyBind, kPsxInputCount>, kPsxKeyboardPlayers> player;
    static KeyboardBinds defaults();
};

bool load_keyboard_binds(const std::string& path, KeyboardBinds* out);  // defaults if missing
bool save_keyboard_binds(const std::string& path, const KeyboardBinds& b);

struct PadMapping {
    std::array<std::string, kPsxInputCount> source;  // "a" / "dpup, a" / ""
    static PadMapping defaults();
};

struct PadBinds {
    bool enabled = true;
    int device = 0;
    int deadzone = 3277;
    PadMapping global;
    std::vector<std::pair<std::string, PadMapping>> per_guid;  // [mapping.<guid>]
    PadMapping* for_guid(const std::string& guid, bool create);
};

bool load_pad_binds(const std::string& path, PadBinds* out);
bool save_pad_binds(const std::string& path, const PadBinds& b);

// Host shortcut keys in config.ini [KeyMap].
extern const char* const kHostShortcutKeys[];
extern const int kHostShortcutKeyCount;
bool load_hotkeys(const std::string& path, std::vector<std::pair<std::string, std::string>>* out);
bool save_hotkeys(const std::string& path, const std::vector<std::pair<std::string, std::string>>& keys);

// RECOMP_LAUNCHER_PAD_* encoding helpers (assist_pad_bind values).
std::string describe_pad_value(int value);

// Sibling path helper: input.ini lives beside keybinds.ini.
std::string sibling_path(const std::string& file, const char* name);

}  // namespace r4l
