// skin_model.h — the data side of a skin package (docs/SKIN_SCHEMA.md).
//
// A skin is a folder with skin.json plus assets. This file parses it into
// plain structs and resolves layout rectangles for a viewport; no GL here, so
// it is unit-tested directly. ui/skin.cpp turns it into textures and draws.
#pragma once

#include "json.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace r4l {

struct Color {
    float r = 1, g = 1, b = 1, a = 1;
};
bool parse_color(const std::string& s, Color* out);  // "#rgb", "#rrggbb", "#rrggbbaa"

// A length: units of the reference height, "N%"/"Nvw"/"Nvh", or "fill[-N]".
struct Length {
    enum Kind { Units, PercentW, PercentH, Fill } kind = Units;
    float v = 0;
};

struct RectSpec {
    std::string anchor = "top-left";  // top-left top top-right left center right bottom-left bottom bottom-right
    Length x, y, w, h;
    bool present = false;
};

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
};

struct Viewport {
    float w = 1280, h = 720;
    float base_h = 720;  // skin.base_height
    float scale() const { return h / base_h; }
};
Rect resolve(const RectSpec& r, const Viewport& vp, const Rect* parent = nullptr);

struct TextStyle {
    std::string ttf;        // path inside the skin (or $exe/...)
    std::string bitmap;     // BMFont .fnt (text format) inside the skin
    float size = 18;        // reference units
    Color color;
    bool shadow = false;
    float shadow_dx = 0, shadow_dy = 2;
    Color shadow_color{0, 0, 0, 0.6f};
    float outline = 0;
    Color outline_color{0, 0, 0, 1};
    bool uppercase = false;
    bool italic_skew = false;  // faux italic for bitmap/ttf text drawn by the skin
};

struct Layer {
    std::string type = "solid";  // solid gradient image stripes streaks
    std::vector<Color> colors;   // solid: [c]; gradient: [top,bottom] or [tl,tr,br,bl]
    std::string file;
    std::string fit = "cover";   // cover contain stretch tile
    float scroll_x = 0, scroll_y = 0;  // units/second
    float opacity = 1;
    float angle = 0, spacing = 40, width = 8;  // stripes
    int count = 24;                            // streaks
    float y_min = 0, y_max = 100;              // streaks band, % of height
    std::vector<std::string> screens;          // empty = all
    bool optional = false;                     // missing runtime asset is not an error
};

struct Sprite {
    std::string file;
    std::string shape;  // "chevron", "bar", "circle" when no file
    RectSpec rect;
    Color tint;
    float opacity = 1;
    float bob_hz = 0, bob_px = 0, spin_hz = 0;
    std::vector<std::string> screens;
    bool optional = false;
};

struct Highlight {
    std::string style = "bar";  // bar box fill glow sprite
    Color color;
    float thickness = 4, radius = 8, pulse_hz = 0;
    std::string sprite;
};

struct CursorSpec {
    std::string sprite;
    std::string shape = "";  // "chevron" when no sprite
    float w = 0, h = 0, offset_x = -8, offset_y = 0, bob_hz = 0, bob_px = 0;
    Color tint;
};

struct Transition {
    std::string type = "fade";  // none fade slide-left slide-up
    float ms = 160;
};

struct SkinModel {
    std::string dir;
    std::string name = "Default";
    std::string author;
    float base_height = 720;
    std::map<std::string, Color> palette;      // bg surface surface_hi line text text_dim accent accent2 ok warn bad
    std::map<std::string, TextStyle> fonts;    // body heading nav button label
    std::map<std::string, RectSpec> layout;    // rail content footer brand play_button home.hero ...
    std::map<std::string, float> metrics;      // row_height radius nav_item_height ...
    std::vector<Layer> background;
    std::vector<Sprite> sprites;
    Highlight highlight;
    CursorSpec cursor;
    Transition transition;
    std::map<std::string, std::string> sounds; // move confirm back
    float sound_volume = 0.6f;
    std::map<std::string, std::string> text;   // label overrides: "nav.Home" -> "RACE"
    std::vector<std::string> warnings;

    float metric(const std::string& k, float fallback) const;
    Color color(const std::string& k, Color fallback) const;
    std::string label(const std::string& key, const std::string& fallback) const;
    bool on_screen(const std::vector<std::string>& screens, const std::string& screen) const;
};

// Load <dir>/skin.json. "extends": "<other skin dir>" merges a base first.
bool load_skin_model(const std::string& dir, SkinModel* out, std::string* err);
std::string resolve_asset(const SkinModel& m, const std::string& path);  // skin-relative, $exe/, $skins/
int64_t skin_mtime(const SkinModel& m);  // newest mtime of skin.json (+ base)

}  // namespace r4l
