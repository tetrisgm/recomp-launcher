#include "skin.h"

#include "r4l/core/session.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>

#define STB_IMAGE_STATIC  // the host may link its own stb_image (psxrecomp does)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#define STBI_ONLY_JPEG
#include "stb_image.h"

namespace r4l {

namespace {
Skin g_skin;
ImU32 u32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }
ImU32 u32c(const Color& c, float a = 1) { return u32(ImVec4(c.r, c.g, c.b, c.a * a)); }

std::string upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

bool parse_fnt(const std::string& path, BitmapFont* f, std::string* page) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    auto val = [](const std::string& l, const char* key) -> std::string {
        const std::string k = std::string(" ") + key + "=";
        size_t p = l.find(k);
        if (p == std::string::npos) return "";
        p += k.size();
        if (l[p] == '"') return l.substr(p + 1, l.find('"', p + 1) - p - 1);
        return l.substr(p, l.find(' ', p) - p);
    };
    while (std::getline(in, line)) {
        line = " " + line;
        if (line.rfind(" info", 0) == 0) f->size = std::fabs(std::stof(val(line, "size") + "0") / 10.0f);
        else if (line.rfind(" common", 0) == 0) {
            f->line_h = std::stof(val(line, "lineHeight"));
            f->base = std::stof(val(line, "base"));
        } else if (line.rfind(" page", 0) == 0) *page = val(line, "file");
        else if (line.rfind(" char ", 0) == 0) {
            BitmapFont::Glyph g{std::stof(val(line, "x")),       std::stof(val(line, "y")),
                                std::stof(val(line, "width")),   std::stof(val(line, "height")),
                                std::stof(val(line, "xoffset")), std::stof(val(line, "yoffset")),
                                std::stof(val(line, "xadvance"))};
            f->glyphs[static_cast<unsigned>(std::stoul(val(line, "id")))] = g;
        }
    }
    return !f->glyphs.empty();
}
}  // namespace

Skin& skin() { return g_skin; }
ImVec4 to_im(const Color& c) { return ImVec4(c.r, c.g, c.b, c.a); }

unsigned Skin::texture(const std::string& rel, int* w, int* h, bool optional) {
    if (rel.empty()) return 0;
    const std::string path = resolve_asset(m_, rel);
    auto it = tex_.find(path);
    if (it == tex_.end()) {
        int x = 0, y = 0, n = 0;
        unsigned char* px = stbi_load(path.c_str(), &x, &y, &n, 4);
        Tex t{0, 0, 0};
        if (px) {
            GLuint id = 0;
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
            stbi_image_free(px);
            t = Tex{id, x, y};
        } else if (!optional) {
            m_.warnings.push_back("missing image " + rel);
        }
        it = tex_.emplace(path, t).first;
    }
    if (w) *w = it->second.w;
    if (h) *h = it->second.h;
    return it->second.id;
}

void Skin::unload() {
    for (auto& kv : tex_)
        if (kv.second.id) {
            GLuint id = kv.second.id;
            glDeleteTextures(1, &id);
        }
    tex_.clear();
    if (ImGui::GetCurrentContext()) {
        std::set<ImFont*> done;
        for (auto& kv : fonts_)
            if (kv.second && done.insert(kv.second).second && kv.second != ImGui::GetIO().Fonts->Fonts[0])
                ImGui::GetIO().Fonts->RemoveFont(kv.second);
    }
    fonts_.clear();
    bitmaps_.clear();
    for (auto& kv : sounds_) SDL_free(kv.second.buf), std::free(kv.second.spec);
    sounds_.clear();
    if (audio_stream_) SDL_DestroyAudioStream(static_cast<SDL_AudioStream*>(audio_stream_));
    audio_stream_ = nullptr;
    loaded_ = false;
}

bool Skin::build() {
    ImGuiIO& io = ImGui::GetIO();
    std::map<std::string, ImFont*> by_file;
    for (const auto& kv : m_.fonts) {
        const TextStyle& t = kv.second;
        if (!t.bitmap.empty()) {
            BitmapFont bf;
            std::string page;
            const std::string fnt = resolve_asset(m_, t.bitmap);
            if (parse_fnt(fnt, &bf, &page)) {
                int w = 0, h = 0;
                bf.tex = texture(join_path(dir_of(t.bitmap.empty() ? "" : t.bitmap), page), &w, &h);
                bf.tw = static_cast<float>(w ? w : 1);
                bf.th = static_cast<float>(h ? h : 1);
                if (bf.tex) bitmaps_[kv.first] = bf;
            } else {
                m_.warnings.push_back("cannot read bitmap font " + t.bitmap);
            }
        }
        if (!t.ttf.empty()) {
            const std::string p = resolve_asset(m_, t.ttf);
            ImFont* f = by_file.count(p) ? by_file[p] : nullptr;
            if (!f && std::ifstream(p)) f = io.Fonts->AddFontFromFileTTF(p.c_str(), 32.0f);
            if (!f) m_.warnings.push_back("cannot load font " + t.ttf);
            by_file[p] = f;
            fonts_[kv.first] = f;
        }
    }
    if (!std::getenv("R4L_NO_SOUND") && !m_.sounds.empty()) {
        SDL_InitSubSystem(SDL_INIT_AUDIO);
        for (const auto& kv : m_.sounds) {
            Sound s;
            SDL_AudioSpec* spec = static_cast<SDL_AudioSpec*>(std::calloc(1, sizeof(SDL_AudioSpec)));
            Uint32 len = 0;
            if (SDL_LoadWAV(resolve_asset(m_, kv.second).c_str(), spec, &s.buf, &len)) {
                s.len = len;
                s.spec = spec;
                sounds_[kv.first] = s;
                if (!audio_stream_)
                    audio_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, spec, nullptr, nullptr);
            } else {
                std::free(spec);
                m_.warnings.push_back("cannot load sound " + kv.second);
            }
        }
        if (audio_stream_) {
            SDL_SetAudioStreamGain(static_cast<SDL_AudioStream*>(audio_stream_), m_.sound_volume);
            SDL_ResumeAudioStreamDevice(static_cast<SDL_AudioStream*>(audio_stream_));
        }
    }
    for (const auto& l : m_.background) if (l.type == "image") texture(l.file, nullptr, nullptr, l.optional);
    for (const auto& s : m_.sprites) if (!s.file.empty()) texture(s.file, nullptr, nullptr, s.optional);
    if (!m_.cursor.sprite.empty()) texture(m_.cursor.sprite);
    if (!m_.highlight.sprite.empty()) texture(m_.highlight.sprite);
    return true;
}

bool Skin::load(const std::string& dir, std::string* err) {
    SkinModel m;
    if (!load_skin_model(dir, &m, err)) return false;
    unload();
    m_ = std::move(m);
    dir_ = dir;
    mtime_ = skin_mtime(m_);
    build();
    loaded_ = true;
    return true;
}

bool Skin::maybe_reload(double now, std::string* err) {
    if (!loaded_ || now < next_poll_) return false;
    next_poll_ = now + 0.5;
    const int64_t mt = skin_mtime(m_);
    if (mt == mtime_) return false;
    mtime_ = mt;
    std::string e;
    if (!load(dir_, &e)) {
        if (err) *err = e;  // previous skin stays active
        return true;
    }
    return true;
}

void Skin::begin_frame(float w, float h, const std::string& screen, double time) {
    vp_.w = w;
    vp_.h = h;
    vp_.base_h = m_.base_height > 0 ? m_.base_height : 720;
    time_ = time;
    if (screen != screen_) {
        screen_ = screen;
        screen_since_ = time;
    }
}

bool Skin::has_rect(const std::string& name) const {
    auto it = m_.layout.find(name + "@" + screen_);
    if (it == m_.layout.end()) it = m_.layout.find(name);
    return it != m_.layout.end() && it->second.present;
}

Rect Skin::rect(const std::string& name, const Rect& fb, const Rect* parent) const {
    // "<region>@<Screen>" overrides a region on one screen (e.g. rail@Home).
    auto it = m_.layout.find(name + "@" + screen_);
    if (it == m_.layout.end() || !it->second.present) it = m_.layout.find(name);
    if (it == m_.layout.end() || !it->second.present) return fb;
    return resolve(it->second, vp_, parent);
}

ImVec4 Skin::color(const std::string& k, const ImVec4& fb) const {
    auto it = m_.palette.find(k);
    return it == m_.palette.end() ? fb : to_im(it->second);
}

void Skin::draw_background(ImDrawList* dl) const {
    const ImVec2 a(0, 0), b(vp_.w, vp_.h);
    const float t = static_cast<float>(time_);
    for (const Layer& l : m_.background) {
        if (!m_.on_screen(l.screens, screen_)) continue;
        if (l.type == "solid" && !l.colors.empty()) {
            dl->AddRectFilled(a, b, u32c(l.colors[0], l.opacity));
        } else if (l.type == "gradient" && l.colors.size() >= 2) {
            const bool four = l.colors.size() >= 4;
            const Color& tl = l.colors[0];
            const Color& tr = four ? l.colors[1] : l.colors[0];
            const Color& br = four ? l.colors[2] : l.colors[1];
            const Color& bl = four ? l.colors[3] : l.colors[1];
            dl->AddRectFilledMultiColor(a, b, u32c(tl, l.opacity), u32c(tr, l.opacity), u32c(br, l.opacity),
                                        u32c(bl, l.opacity));
        } else if (l.type == "stripes") {
            const Color c = l.colors.empty() ? Color{1, 1, 1, 0.05f} : l.colors[0];
            const float sp = l.spacing * s(), wd = l.width * s();
            const float rad = l.angle * 3.14159265f / 180.0f;
            const float dx = std::tan(rad) * vp_.h;
            const float off = std::fmod(t * l.scroll_x * s(), sp);
            for (float x = -std::fabs(dx) - sp + off; x < vp_.w + std::fabs(dx) + sp; x += sp) {
                const ImVec2 p[4] = {ImVec2(x, 0), ImVec2(x + wd, 0), ImVec2(x + wd - dx, vp_.h), ImVec2(x - dx, vp_.h)};
                dl->AddConvexPolyFilled(p, 4, u32c(c, l.opacity));
            }
        } else if (l.type == "streaks") {
            // Horizontal light streaks (racing-game title backdrops): fixed
            // pseudo-random rows, each a bar fading in from the left and out
            // to the right, drifting at its own speed. Deterministic per index.
            uint32_t seed = 2463534242u;
            auto rnd = [&seed]() {
                seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
                return (seed & 0xffffff) / 16777216.0f;
            };
            for (int i = 0; i < l.count; ++i) {
                const Color c = l.colors.empty() ? Color{1, 1, 1, 0.5f} : l.colors[i % l.colors.size()];
                const float y = vp_.h * (l.y_min + (l.y_max - l.y_min) * rnd()) / 100.0f;
                const float len = vp_.w * (0.25f + 0.9f * rnd());
                const float th = std::max(1.0f, l.width * s() * (0.3f + rnd()));
                const float speed = (l.scroll_x ? l.scroll_x : 200) * s() * (0.5f + rnd());
                const float span = vp_.w + len;
                const float x = std::fmod(rnd() * span + t * speed, span) - len;
                const ImU32 mid = u32c(c, l.opacity), clear = u32c(Color{c.r, c.g, c.b, 0});
                dl->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + len * 0.7f, y + th), clear, mid, mid, clear);
                dl->AddRectFilledMultiColor(ImVec2(x + len * 0.7f, y), ImVec2(x + len, y + th), mid, clear, clear, mid);
            }
        } else if (l.type == "image") {
            auto it = tex_.find(resolve_asset(m_, l.file));
            if (it == tex_.end() || !it->second.id) continue;
            const float iw = static_cast<float>(it->second.w), ih = static_cast<float>(it->second.h);
            const ImU32 tint = u32c(l.colors.empty() ? Color{} : l.colors[0], l.opacity);
            const ImTextureID id = static_cast<ImTextureID>(it->second.id);
            if (l.fit == "tile") {
                const float tw = iw * s(), th = ih * s();
                const float ox = std::fmod(t * l.scroll_x * s(), tw), oy = std::fmod(t * l.scroll_y * s(), th);
                for (float y = -th + oy; y < vp_.h; y += th)
                    for (float x = -tw + ox; x < vp_.w; x += tw) dl->AddImage(id, ImVec2(x, y), ImVec2(x + tw, y + th), ImVec2(0, 0), ImVec2(1, 1), tint);
            } else {
                float sc = 1;
                if (l.fit == "cover") sc = std::max(vp_.w / iw, vp_.h / ih);
                else if (l.fit == "contain") sc = std::min(vp_.w / iw, vp_.h / ih);
                const ImVec2 sz = l.fit == "stretch" ? ImVec2(vp_.w, vp_.h) : ImVec2(iw * sc, ih * sc);
                const ImVec2 p((vp_.w - sz.x) * 0.5f, (vp_.h - sz.y) * 0.5f);
                const float u = t * l.scroll_x * 0.01f, v = t * l.scroll_y * 0.01f;
                dl->AddImage(id, p, ImVec2(p.x + sz.x, p.y + sz.y), ImVec2(u, v), ImVec2(1 + u, 1 + v), tint);
            }
        }
    }
}

namespace {
void shape(ImDrawList* dl, const std::string& kind, ImVec2 a, ImVec2 b, ImU32 c, float spin) {
    const ImVec2 m((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    const float w = b.x - a.x, h = b.y - a.y;
    if (kind == "chevron") {
        const float t = w * 0.38f;
        const ImVec2 p[6] = {a, ImVec2(a.x + t, a.y), ImVec2(b.x, m.y), ImVec2(a.x + t, b.y), ImVec2(a.x, b.y), ImVec2(b.x - t, m.y)};
        dl->AddConvexPolyFilled(p, 3, c);  // upper half
        const ImVec2 q[4] = {ImVec2(b.x - t, m.y), ImVec2(b.x, m.y), ImVec2(a.x + t, b.y), ImVec2(a.x, b.y)};
        const ImVec2 u[4] = {a, ImVec2(a.x + t, a.y), ImVec2(b.x, m.y), ImVec2(b.x - t, m.y)};
        dl->AddConvexPolyFilled(u, 4, c);
        dl->AddConvexPolyFilled(q, 4, c);
        (void)p;
    } else if (kind == "circle") {
        dl->AddCircleFilled(m, std::min(w, h) * 0.5f, c, 48);
    } else if (kind == "slant") {
        const ImVec2 p[4] = {ImVec2(a.x + h * 0.5f, a.y), b.x > a.x ? ImVec2(b.x, a.y) : a, ImVec2(b.x - h * 0.5f, b.y), ImVec2(a.x, b.y)};
        dl->AddConvexPolyFilled(p, 4, c);
    } else if (kind == "ring") {
        dl->PathArcTo(m, std::min(w, h) * 0.5f, spin, spin + 4.5f, 48);
        dl->PathStroke(c, 0, std::max(2.0f, std::min(w, h) * 0.06f));
    } else {
        dl->AddRectFilled(a, b, c);
    }
}
}  // namespace

void Skin::draw_sprites(ImDrawList* dl) const {
    const float t = static_cast<float>(time_);
    for (const Sprite& sp : m_.sprites) {
        if (!m_.on_screen(sp.screens, screen_)) continue;
        Rect r = resolve(sp.rect, vp_);
        const float bob = sp.bob_hz ? std::sin(t * sp.bob_hz * 6.2831853f) * sp.bob_px * s() : 0;
        const ImVec2 a(r.x, r.y + bob), b(r.x + r.w, r.y + r.h + bob);
        const ImU32 c = u32c(sp.tint, sp.opacity);
        if (!sp.file.empty()) {
            auto it = tex_.find(resolve_asset(m_, sp.file));
            if (it != tex_.end() && it->second.id) dl->AddImage(static_cast<ImTextureID>(it->second.id), a, b, ImVec2(0, 0), ImVec2(1, 1), c);
        } else {
            shape(dl, sp.shape, a, b, c, t * sp.spin_hz * 6.2831853f);
        }
    }
}

void Skin::draw_highlight(ImDrawList* dl, ImVec2 a, ImVec2 b) const {
    const Highlight& h = m_.highlight;
    float pulse = 1;
    if (h.pulse_hz > 0) pulse = 0.75f + 0.25f * std::sin(static_cast<float>(time_) * h.pulse_hz * 6.2831853f);
    const ImU32 c = u32c(h.color, pulse);
    const float r = h.radius * s(), th = h.thickness * s();
    if (h.style == "box") dl->AddRect(a, b, c, r, 0, th);
    else if (h.style == "fill") dl->AddRectFilled(a, b, c, r);
    else if (h.style == "slant") shape(dl, "slant", a, b, c, 0);
    else if (h.style == "glow") {
        for (int i = 3; i >= 1; --i)
            dl->AddRect(ImVec2(a.x - i * 2, a.y - i * 2), ImVec2(b.x + i * 2, b.y + i * 2), u32c(h.color, pulse * 0.18f * i), r + i * 2.0f, 0, 2.0f);
        dl->AddRectFilled(a, b, u32c(h.color, 0.18f * pulse), r);
    } else if (h.style == "sprite" && !h.sprite.empty()) {
        auto it = tex_.find(resolve_asset(m_, h.sprite));
        if (it != tex_.end() && it->second.id) dl->AddImage(static_cast<ImTextureID>(it->second.id), a, b, ImVec2(0, 0), ImVec2(1, 1), c);
    } else {  // bar
        dl->AddRectFilled(a, b, u32c(h.color, 0.14f * pulse), r);
        dl->AddRectFilled(ImVec2(a.x, a.y + (b.y - a.y) * 0.2f), ImVec2(a.x + th, b.y - (b.y - a.y) * 0.2f), c, th * 0.5f);
    }
}

void Skin::draw_cursor(ImDrawList* dl, ImVec2 a, ImVec2 b) const {
    const CursorSpec& k = m_.cursor;
    if (k.sprite.empty() && k.shape.empty()) return;
    const float w = (k.w ? k.w : 16) * s(), h = (k.h ? k.h : 16) * s();
    const float bob = k.bob_hz ? std::sin(static_cast<float>(time_) * k.bob_hz * 6.2831853f) * k.bob_px * s() : 0;
    const float x = a.x + k.offset_x * s() - w + bob;
    const float y = (a.y + b.y) * 0.5f - h * 0.5f + k.offset_y * s();
    if (!k.sprite.empty()) {
        auto it = tex_.find(resolve_asset(m_, k.sprite));
        if (it != tex_.end() && it->second.id)
            dl->AddImage(static_cast<ImTextureID>(it->second.id), ImVec2(x, y), ImVec2(x + w, y + h), ImVec2(0, 0), ImVec2(1, 1), u32c(k.tint));
    } else {
        shape(dl, k.shape, ImVec2(x, y), ImVec2(x + w, y + h), u32c(k.tint), 0);
    }
}

ImFont* Skin::imfont(const std::string& role) const {
    auto it = fonts_.find(role);
    if (it != fonts_.end() && it->second) return it->second;
    it = fonts_.find("body");
    return it != fonts_.end() ? it->second : nullptr;
}

float Skin::font_px(const std::string& role) const {
    auto it = m_.fonts.find(role);
    if (it == m_.fonts.end()) it = m_.fonts.find("body");
    return (it == m_.fonts.end() ? 18.0f : it->second.size) * s();
}

ImVec2 Skin::text_size(const std::string& role, const std::string& str) const {
    auto st = m_.fonts.find(role);
    const std::string t = (st != m_.fonts.end() && st->second.uppercase) ? upper(str) : str;
    auto bm = bitmaps_.find(role);
    const float px = font_px(role);
    if (bm != bitmaps_.end()) {
        const BitmapFont& f = bm->second;
        const float sc = px / (f.size > 0 ? f.size : f.line_h);
        float w = 0;
        for (unsigned char c : t) {
            auto g = f.glyphs.find(c);
            if (g != f.glyphs.end()) w += g->second.adv * sc;
        }
        return ImVec2(w, f.line_h * sc);
    }
    ImFont* f = imfont(role);
    if (!f) f = ImGui::GetFont();
    return f->CalcTextSizeA(px, FLT_MAX, 0, t.c_str());
}

void Skin::text(ImDrawList* dl, const std::string& role, ImVec2 pos, const std::string& str, float align,
                const ImVec4* override_color) const {
    auto st = m_.fonts.find(role);
    TextStyle style = st != m_.fonts.end() ? st->second
                                           : (m_.fonts.count("body") ? m_.fonts.at("body") : TextStyle{});
    const std::string t = style.uppercase ? upper(str) : str;
    const ImVec2 sz = text_size(role, str);
    pos.x -= sz.x * align;
    const float px = font_px(role);
    const ImU32 main = override_color ? u32(*override_color) : u32c(style.color);
    auto bm = bitmaps_.find(role);
    auto draw_at = [&](ImVec2 p, ImU32 c) {
        if (bm != bitmaps_.end()) {
            const BitmapFont& f = bm->second;
            const float sc = px / (f.size > 0 ? f.size : f.line_h);
            float x = p.x;
            for (unsigned char ch : t) {
                auto g = f.glyphs.find(ch);
                if (g == f.glyphs.end()) continue;
                const auto& G = g->second;
                const ImVec2 a(x + G.xoff * sc, p.y + G.yoff * sc), b(a.x + G.w * sc, a.y + G.h * sc);
                dl->AddImage(static_cast<ImTextureID>(f.tex), a, b, ImVec2(G.x / f.tw, G.y / f.th),
                             ImVec2((G.x + G.w) / f.tw, (G.y + G.h) / f.th), c);
                x += G.adv * sc;
            }
        } else {
            ImFont* f = imfont(role);
            if (!f) f = ImGui::GetFont();
            if (style.italic_skew) {
                const int v0 = dl->VtxBuffer.Size;
                dl->AddText(f, px, p, c, t.c_str());
                for (int i = v0; i < dl->VtxBuffer.Size; ++i) {
                    ImDrawVert& v = dl->VtxBuffer[i];
                    v.pos.x += (p.y + px - v.pos.y) * 0.18f;
                }
            } else {
                dl->AddText(f, px, p, c, t.c_str());
            }
        }
    };
    if (style.shadow) draw_at(ImVec2(pos.x + style.shadow_dx * s(), pos.y + style.shadow_dy * s()), u32c(style.shadow_color));
    if (style.outline > 0) {
        const float o = std::max(1.0f, style.outline * s());
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (dx || dy) draw_at(ImVec2(pos.x + dx * o, pos.y + dy * o), u32c(style.outline_color));
    }
    draw_at(pos, main);
}

float Skin::transition_alpha() const {
    if (m_.transition.type == "none" || m_.transition.ms <= 0 || std::getenv("R4L_NO_TRANSITION")) return 1;
    const float k = static_cast<float>((time_ - screen_since_) * 1000.0 / m_.transition.ms);
    return k >= 1 ? 1.0f : (k < 0 ? 0.0f : k);
}

ImVec2 Skin::transition_offset() const {
    const float k = transition_alpha();
    const float e = 1 - (1 - k) * (1 - k);
    const float d = (1 - e) * 40 * s();
    if (m_.transition.type == "slide-left") return ImVec2(d, 0);
    if (m_.transition.type == "slide-up") return ImVec2(0, d);
    return ImVec2(0, 0);
}

void Skin::play(const std::string& name) const {
    auto it = sounds_.find(name);
    if (it == sounds_.end() || !audio_stream_) return;
    SDL_AudioStream* st = static_cast<SDL_AudioStream*>(audio_stream_);
    SDL_ClearAudioStream(st);
    SDL_PutAudioStreamData(st, it->second.buf, static_cast<int>(it->second.len));
}

}  // namespace r4l
