#include "binds.h"

#include "ini.h"
#include "recomp_launcher.h"

#include <cstdio>

namespace r4l {

const PsxInput kPsxInputs[kPsxInputCount] = {
    {"up", "D-pad Up"},        {"down", "D-pad Down"},     {"left", "D-pad Left"},
    {"right", "D-pad Right"},  {"cross", "Cross"},         {"circle", "Circle"},
    {"square", "Square"},      {"triangle", "Triangle"},   {"l1", "L1"},
    {"r1", "R1"},              {"l2", "L2"},               {"r2", "R2"},
    {"l3", "L3"},              {"r3", "R3"},               {"start", "Start"},
    {"select", "Select"},      {"ls_up", "Left stick Up"}, {"ls_down", "Left stick Down"},
    {"ls_left", "Left stick Left"},  {"ls_right", "Left stick Right"},
    {"rs_up", "Right stick Up"},     {"rs_down", "Right stick Down"},
    {"rs_left", "Right stick Left"}, {"rs_right", "Right stick Right"},
};

namespace {

// Runtime defaults (psx_keybinds.c PSXKB_PLAYER_DEFAULTS), SDL scancode names.
const char* const kKeyDefaults[kPsxInputCount] = {
    "Up", "Down", "Left", "Right", "X", "S", "Z", "A", "Q", "W", "E", "R",
    "T", "Y", "Return", "Right Shift", "Up", "Down", "Left", "Right", "", "", "", "",
};

// Runtime defaults (main.cpp default_input_ini_text).
const char* const kPadDefaults[kPsxInputCount] = {
    "dpup", "dpdown", "dpleft", "dpright", "a", "b", "x", "y",
    "leftshoulder", "rightshoulder", "lefttrigger", "righttrigger",
    "leftstick", "rightstick", "start", "back",
    "lefty-", "lefty+", "leftx-", "leftx+", "righty-", "righty+", "rightx-", "rightx+",
};

void split_pair(const std::string& v, std::string* a, std::string* b) {
    const size_t c = v.find(',');
    *a = trim(c == std::string::npos ? v : v.substr(0, c));
    *b = c == std::string::npos ? "" : trim(v.substr(c + 1));
    if (iequals(*a, "None")) a->clear();
    if (iequals(*b, "None")) b->clear();
}

void read_mapping(const IniDoc& d, const std::string& sec, PadMapping* m) {
    for (int i = 0; i < kPsxInputCount; ++i)
        if (d.has(sec, kPsxInputs[i].key)) m->source[i] = d.get(sec, kPsxInputs[i].key);
}

void write_mapping(IniDoc& d, const std::string& sec, const PadMapping& m) {
    for (int i = 0; i < kPsxInputCount; ++i)
        d.set(sec, kPsxInputs[i].key, m.source[i].empty() ? "none" : m.source[i]);
}

}  // namespace

KeyboardBinds KeyboardBinds::defaults() {
    KeyboardBinds b;
    for (auto& p : b.player)
        for (int i = 0; i < kPsxInputCount; ++i) p[i] = KeyBind{kKeyDefaults[i], ""};
    return b;
}

bool load_keyboard_binds(const std::string& path, KeyboardBinds* out) {
    *out = KeyboardBinds::defaults();
    IniDoc d;
    if (!d.load(path)) return false;
    for (int p = 0; p < kPsxKeyboardPlayers; ++p) {
        const std::string sec = "player" + std::to_string(p + 1);
        for (int i = 0; i < kPsxInputCount; ++i) {
            if (!d.has(sec, kPsxInputs[i].key)) continue;
            split_pair(d.get(sec, kPsxInputs[i].key), &out->player[p][i].primary,
                       &out->player[p][i].alt);
        }
    }
    return true;
}

bool save_keyboard_binds(const std::string& path, const KeyboardBinds& b) {
    IniDoc d;
    d.load(path);  // keep comments and any keys we do not own
    for (int p = 0; p < kPsxKeyboardPlayers; ++p) {
        const std::string sec = "player" + std::to_string(p + 1);
        for (int i = 0; i < kPsxInputCount; ++i) {
            const KeyBind& k = b.player[p][i];
            std::string v = k.primary.empty() ? "None" : k.primary;
            if (!k.alt.empty()) v += ", " + k.alt;
            d.set(sec, kPsxInputs[i].key, v);
        }
    }
    return d.save(path);
}

PadMapping PadMapping::defaults() {
    PadMapping m;
    for (int i = 0; i < kPsxInputCount; ++i) m.source[i] = kPadDefaults[i];
    return m;
}

PadMapping* PadBinds::for_guid(const std::string& guid, bool create) {
    if (guid.empty()) return &global;
    for (auto& e : per_guid)
        if (iequals(e.first, guid)) return &e.second;
    if (!create) return &global;
    per_guid.emplace_back(guid, global);
    return &per_guid.back().second;
}

bool load_pad_binds(const std::string& path, PadBinds* out) {
    *out = PadBinds{};
    out->global = PadMapping::defaults();
    IniDoc d;
    if (!d.load(path)) return false;
    out->enabled = !iequals(d.get("controller", "enabled", "true"), "false");
    out->device = d.get_int("controller", "device", 0);
    out->deadzone = d.get_int("controller", "deadzone", 3277);
    read_mapping(d, "mapping", &out->global);
    for (const std::string& s : d.sections()) {
        if (s.rfind("mapping.", 0) != 0) continue;
        PadMapping m = out->global;
        read_mapping(d, s, &m);
        out->per_guid.emplace_back(s.substr(8), m);
    }
    return true;
}

bool save_pad_binds(const std::string& path, const PadBinds& b) {
    IniDoc d;
    d.load(path);
    d.set("controller", "enabled", b.enabled ? "true" : "false");
    d.set("controller", "device", std::to_string(b.device));
    d.set("controller", "deadzone", std::to_string(b.deadzone));
    write_mapping(d, "mapping", b.global);
    for (const auto& e : b.per_guid) write_mapping(d, "mapping." + e.first, e.second);
    return d.save(path);
}

const char* const kHostShortcutKeys[] = {
    "Fullscreen", "Reset", "Pause", "Turbo", "TurboToggle", "Rewind", "SaveStateMenu",
    "VolumeUp", "VolumeDown", "DisplayPerf", "OpenLauncher",
};
const int kHostShortcutKeyCount =
    static_cast<int>(sizeof(kHostShortcutKeys) / sizeof(kHostShortcutKeys[0]));

bool load_hotkeys(const std::string& path, std::vector<std::pair<std::string, std::string>>* out) {
    out->clear();
    IniDoc d;
    const bool ok = d.load(path);
    for (int i = 0; i < kHostShortcutKeyCount; ++i)
        out->emplace_back(kHostShortcutKeys[i], d.get("KeyMap", kHostShortcutKeys[i]));
    return ok;
}

bool save_hotkeys(const std::string& path,
                  const std::vector<std::pair<std::string, std::string>>& keys) {
    IniDoc d;
    d.load(path);
    for (const auto& kv : keys)
        if (!kv.second.empty() || d.has("KeyMap", kv.first)) d.set("KeyMap", kv.first, kv.second);
    return d.save(path);
}

std::string describe_pad_value(int v) {
    static const char* const kButtons[] = {
        "A", "B", "X", "Y", "Back", "Guide", "Start", "L3", "R3", "LB", "RB",
        "Up", "Down", "Left", "Right", "Misc", "P1", "P2", "P3", "P4", "Touchpad",
    };
    const int nb = static_cast<int>(sizeof(kButtons) / sizeof(kButtons[0]));
    if (v <= 0) return "Unbound";
    if (RECOMP_LAUNCHER_PAD_IS_BUTTON(v)) {
        const int c = RECOMP_LAUNCHER_PAD_BUTTON_CODE(v);
        return c < nb ? kButtons[c] : "Button " + std::to_string(c);
    }
    if (RECOMP_LAUNCHER_PAD_IS_AXIS(v)) {
        static const char* const kAxes[] = {"LX", "LY", "RX", "RY", "LT", "RT"};
        const int a = RECOMP_LAUNCHER_PAD_AXIS_CODE(v);
        std::string s = a < 6 ? kAxes[a] : "Axis " + std::to_string(a);
        return s + (RECOMP_LAUNCHER_PAD_AXIS_POSITIVE(v) ? "+" : "-");
    }
    const int mask = RECOMP_LAUNCHER_PAD_BUTTON_COMBO_MASK(v);
    std::string s;
    for (int b = 0; b < nb; ++b)
        if (mask & (1 << b)) s += (s.empty() ? "" : " + ") + std::string(kButtons[b]);
    return s.empty() ? "Unbound" : s;
}

std::string sibling_path(const std::string& file, const char* name) {
    const size_t slash = file.find_last_of("/\\");
    if (slash == std::string::npos) return name;
    return file.substr(0, slash + 1) + name;
}

}  // namespace r4l
