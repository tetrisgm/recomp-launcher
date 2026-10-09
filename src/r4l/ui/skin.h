// skin.h — runtime side of a skin: textures, fonts, sounds, drawing.
#pragma once

#include "r4l/core/skin_model.h"

#include "imgui.h"

#include <map>
#include <string>

namespace r4l {

struct BitmapFont {
    struct Glyph {
        float x, y, w, h, xoff, yoff, adv;
    };
    unsigned tex = 0;
    float tw = 1, th = 1, line_h = 16, base = 12, size = 16;
    std::map<unsigned, Glyph> glyphs;
};

class Skin {
public:
    bool load(const std::string& dir, std::string* err);  // keeps previous skin on error
    void unload();
    // Polls skin.json mtime (twice a second); reloads in place when changed.
    // Returns true when a reload happened (err set on failure).
    bool maybe_reload(double now, std::string* err);
    bool loaded() const { return loaded_; }
    const SkinModel& model() const { return m_; }

    void begin_frame(float w, float h, const std::string& screen, double time);
    float s() const { return vp_.scale(); }                 // reference units -> pixels
    Rect rect(const std::string& name, const Rect& fallback, const Rect* parent = nullptr) const;
    bool has_rect(const std::string& name) const;
    float metric(const std::string& k, float fb) const { return m_.metric(k, fb) * vp_.scale(); }
    ImVec4 color(const std::string& k, const ImVec4& fb) const;
    std::string label(const std::string& k, const std::string& fb) const { return m_.label(k, fb); }

    void draw_background(ImDrawList* dl) const;
    void draw_sprites(ImDrawList* dl) const;
    void draw_highlight(ImDrawList* dl, ImVec2 a, ImVec2 b) const;
    void draw_cursor(ImDrawList* dl, ImVec2 a, ImVec2 b) const;
    // Text in a skin role (heading, nav, button, label, ...). align: 0 left, 0.5 centre, 1 right.
    ImVec2 text_size(const std::string& role, const std::string& s) const;
    void text(ImDrawList* dl, const std::string& role, ImVec2 pos, const std::string& s,
              float align = 0, const ImVec4* color_override = nullptr) const;
    ImFont* imfont(const std::string& role) const;
    float font_px(const std::string& role) const;

    // Transition progress for the current screen: alpha and offset in px.
    float transition_alpha() const;
    ImVec2 transition_offset() const;
    void play(const std::string& sound) const;

private:
    unsigned texture(const std::string& path, int* w = nullptr, int* h = nullptr, bool optional = false);
    bool build();

    SkinModel m_;
    bool loaded_ = false;
    std::string dir_;
    int64_t mtime_ = 0;
    double next_poll_ = 0;
    Viewport vp_;
    std::string screen_;
    double time_ = 0, screen_since_ = 0;
    struct Tex { unsigned id; int w, h; };
    std::map<std::string, Tex> tex_;
    std::map<std::string, ImFont*> fonts_;
    std::map<std::string, BitmapFont> bitmaps_;
    struct Sound { unsigned char* buf = nullptr; unsigned len = 0; void* spec = nullptr; };
    std::map<std::string, Sound> sounds_;
    void* audio_stream_ = nullptr;
};

Skin& skin();  // the launcher's active skin
ImVec4 to_im(const Color& c);

}  // namespace r4l
