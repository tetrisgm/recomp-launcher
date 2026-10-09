#include "skin_model.h"

#include "session.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace r4l {

bool parse_color(const std::string& s, Color* out) {
    if (s.empty() || s[0] != '#') return false;
    auto hx = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::string h = s.substr(1);
    if (h.size() == 3 || h.size() == 4) {
        std::string x;
        for (char c : h) x += std::string(2, c);
        h = x;
    }
    if (h.size() != 6 && h.size() != 8) return false;
    int v[4] = {0, 0, 0, 255};
    for (size_t i = 0; i < h.size() / 2; ++i) {
        const int a = hx(h[2 * i]), b = hx(h[2 * i + 1]);
        if (a < 0 || b < 0) return false;
        v[i] = a * 16 + b;
    }
    *out = Color{v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f, v[3] / 255.0f};
    return true;
}

namespace {

Length parse_len(const Json& j) {
    Length l;
    if (j.is(Json::Number)) {
        l.v = static_cast<float>(j.n);
    } else if (j.is(Json::String)) {
        const std::string s = j.s;
        if (s.rfind("fill", 0) == 0) { l.kind = Length::Fill; l.v = s.size() > 5 ? std::stof(s.substr(5)) : 0; }
        else if (!s.empty() && s.back() == '%') { l.kind = Length::PercentW; l.v = std::stof(s); }
        else if (s.size() > 2 && s.substr(s.size() - 2) == "vh") { l.kind = Length::PercentH; l.v = std::stof(s); }
        else if (s.size() > 2 && s.substr(s.size() - 2) == "vw") { l.kind = Length::PercentW; l.v = std::stof(s); }
        else l.v = std::stof(s);
    }
    return l;
}

RectSpec parse_rect(const Json& j) {
    RectSpec r;
    if (!j.is(Json::Object)) return r;
    r.present = true;
    r.anchor = j["anchor"].str("top-left");
    r.x = parse_len(j["x"]);
    r.y = parse_len(j["y"]);
    r.w = parse_len(j["w"]);
    r.h = parse_len(j["h"]);
    return r;
}

Color col(const Json& j, Color fb, std::vector<std::string>* warn, const char* what) {
    Color c = fb;
    if (j.is(Json::String) && !parse_color(j.s, &c) && warn) warn->push_back(std::string("bad colour in ") + what);
    return c;
}

std::vector<std::string> strs(const Json& j) {
    std::vector<std::string> v;
    for (const Json& e : j.arr) v.push_back(e.str());
    return v;
}

TextStyle parse_text(const Json& j, const TextStyle& base, std::vector<std::string>* w) {
    TextStyle t = base;
    if (const Json* f = j.get("ttf")) t.ttf = f->str();
    if (const Json* f = j.get("bitmap")) t.bitmap = f->str();
    t.size = static_cast<float>(j["size"].num(t.size));
    t.color = col(j["color"], t.color, w, "font colour");
    t.uppercase = j["uppercase"].boolean(t.uppercase);
    t.italic_skew = j["italic"].boolean(t.italic_skew);
    if (const Json* s = j.get("shadow")) {
        t.shadow = s->is(Json::Object);
        t.shadow_dx = static_cast<float>((*s)["dx"].num(t.shadow_dx));
        t.shadow_dy = static_cast<float>((*s)["dy"].num(t.shadow_dy));
        t.shadow_color = col((*s)["color"], t.shadow_color, w, "shadow");
    }
    if (const Json* o = j.get("outline")) {
        t.outline = static_cast<float>((*o)["px"].num(1));
        t.outline_color = col((*o)["color"], t.outline_color, w, "outline");
    }
    return t;
}

bool read_file(const std::string& p, std::string* out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    std::stringstream ss;
    ss << f.rdbuf();
    *out = ss.str();
    return true;
}

void apply_json(const Json& j, SkinModel* m) {
    auto& w = m->warnings;
    m->name = j["name"].str(m->name);
    m->author = j["author"].str(m->author);
    m->base_height = static_cast<float>(j["base_height"].num(m->base_height));
    for (const auto& kv : j["palette"].obj) m->palette[kv.first] = col(kv.second, Color{}, &w, kv.first.c_str());
    for (const auto& kv : j["fonts"].obj) {
        const TextStyle base = m->fonts.count(kv.first) ? m->fonts[kv.first] :
                               m->fonts.count("body") ? m->fonts["body"] : TextStyle{};
        m->fonts[kv.first] = parse_text(kv.second, base, &w);
    }
    for (const auto& kv : j["layout"].obj) m->layout[kv.first] = parse_rect(kv.second);
    for (const auto& kv : j["metrics"].obj) m->metrics[kv.first] = static_cast<float>(kv.second.num(0));
    if (const Json* bg = j.get("background")) {
        m->background.clear();
        for (const Json& l : (*bg)["layers"].arr) {
            Layer L;
            L.type = l["type"].str("solid");
            if (l["colors"].is(Json::Array))
                for (const Json& c : l["colors"].arr) L.colors.push_back(col(c, Color{}, &w, "layer"));
            if (l["color"].is(Json::String)) L.colors.push_back(col(l["color"], Color{}, &w, "layer"));
            L.file = l["file"].str();
            L.fit = l["fit"].str("cover");
            L.scroll_x = static_cast<float>(l["scroll"].at(0).num(0));
            L.scroll_y = static_cast<float>(l["scroll"].at(1).num(0));
            L.opacity = static_cast<float>(l["opacity"].num(1));
            L.angle = static_cast<float>(l["angle"].num(0));
            L.spacing = static_cast<float>(l["spacing"].num(40));
            L.width = static_cast<float>(l["width"].num(8));
            L.count = static_cast<int>(l["count"].num(24));
            L.y_min = static_cast<float>(l["band"].at(0).num(0));
            L.y_max = static_cast<float>(l["band"].at(1).num(100));
            L.screens = strs(l["screens"]);
            L.optional = l["optional"].boolean(false);
            m->background.push_back(L);
        }
    }
    if (const Json* sp = j.get("sprites")) {
        m->sprites.clear();
        for (const Json& s : sp->arr) {
            Sprite S;
            S.file = s["file"].str();
            S.shape = s["shape"].str();
            S.rect = parse_rect(s["rect"]);
            S.tint = col(s["tint"], Color{}, &w, "sprite tint");
            S.opacity = static_cast<float>(s["opacity"].num(1));
            S.bob_hz = static_cast<float>(s["bob_hz"].num(0));
            S.bob_px = static_cast<float>(s["bob_px"].num(0));
            S.spin_hz = static_cast<float>(s["spin_hz"].num(0));
            S.screens = strs(s["screens"]);
            S.optional = s["optional"].boolean(false);
            m->sprites.push_back(S);
        }
    }
    if (const Json* h = j.get("highlight")) {
        m->highlight.style = (*h)["style"].str(m->highlight.style);
        m->highlight.color = col((*h)["color"], m->highlight.color, &w, "highlight");
        m->highlight.thickness = static_cast<float>((*h)["thickness"].num(m->highlight.thickness));
        m->highlight.radius = static_cast<float>((*h)["radius"].num(m->highlight.radius));
        m->highlight.pulse_hz = static_cast<float>((*h)["pulse_hz"].num(m->highlight.pulse_hz));
        m->highlight.sprite = (*h)["sprite"].str(m->highlight.sprite);
    }
    if (const Json* c = j.get("cursor")) {
        CursorSpec& k = m->cursor;
        k.sprite = (*c)["sprite"].str(k.sprite);
        k.shape = (*c)["shape"].str(k.shape);
        k.w = static_cast<float>((*c)["w"].num(k.w));
        k.h = static_cast<float>((*c)["h"].num(k.h));
        k.offset_x = static_cast<float>((*c)["offset_x"].num(k.offset_x));
        k.offset_y = static_cast<float>((*c)["offset_y"].num(k.offset_y));
        k.bob_hz = static_cast<float>((*c)["bob_hz"].num(k.bob_hz));
        k.bob_px = static_cast<float>((*c)["bob_px"].num(k.bob_px));
        k.tint = col((*c)["tint"], k.tint, &w, "cursor tint");
    }
    if (const Json* t = j.get("transition")) {
        m->transition.type = (*t)["type"].str(m->transition.type);
        m->transition.ms = static_cast<float>((*t)["ms"].num(m->transition.ms));
    }
    if (const Json* s = j.get("sounds")) {
        for (const auto& kv : s->obj)
            if (kv.first == "volume") m->sound_volume = static_cast<float>(kv.second.num(0.6));
            else m->sounds[kv.first] = kv.second.str();
    }
    for (const auto& kv : j["text"].obj) m->text[kv.first] = kv.second.str();
}

bool load_into(const std::string& dir, SkinModel* m, std::string* err, int depth) {
    std::string text;
    if (!read_file(join_path(dir, "skin.json"), &text)) {
        if (err) *err = "cannot read " + join_path(dir, "skin.json");
        return false;
    }
    Json j;
    std::string perr;
    if (!parse_json(text, &j, &perr)) {
        if (err) *err = "skin.json " + perr;
        return false;
    }
    if (const Json* ext = j.get("extends")) {
        if (depth > 4) {
            if (err) *err = "extends chain too deep";
            return false;
        }
        std::string base = ext->str();
        if (!base.empty() && base[0] != '/') base = join_path(dir, base);
        if (!load_into(base, m, err, depth + 1)) return false;
    }
    m->dir = dir;
    apply_json(j, m);
    return true;
}

}  // namespace

bool load_skin_model(const std::string& dir, SkinModel* out, std::string* err) {
    SkinModel m;
    if (!load_into(dir, &m, err, 0)) return false;
    *out = std::move(m);
    return true;
}

Rect resolve(const RectSpec& r, const Viewport& vp, const Rect* parent) {
    const Rect P = parent ? *parent : Rect{0, 0, vp.w, vp.h};
    const float s = vp.scale();
    auto len = [&](const Length& l, float span, bool horizontal) -> float {
        switch (l.kind) {
        case Length::PercentW: return (horizontal ? P.w : vp.w) * l.v / 100.0f;
        case Length::PercentH: return vp.h * l.v / 100.0f;
        case Length::Fill: return span - l.v * s;
        default: return l.v * s;
        }
    };
    Rect o;
    o.w = len(r.w, P.w, true);
    o.h = len(r.h, P.h, false);
    const float ox = len(r.x, 0, true), oy = len(r.y, 0, false);
    const std::string& a = r.anchor;
    const bool right = a.find("right") != std::string::npos;
    const bool bottom = a.find("bottom") != std::string::npos;
    const bool hcenter = a == "top" || a == "bottom" || a == "center";
    const bool vcenter = a == "left" || a == "right" || a == "center";
    if (r.w.kind == Length::Fill) o.w = P.w - ox - r.w.v * s;
    if (r.h.kind == Length::Fill) o.h = P.h - oy - r.h.v * s;
    o.x = right ? P.x + P.w - o.w - ox : hcenter ? P.x + (P.w - o.w) * 0.5f + ox : P.x + ox;
    o.y = bottom ? P.y + P.h - o.h - oy : vcenter ? P.y + (P.h - o.h) * 0.5f + oy : P.y + oy;
    return o;
}

float SkinModel::metric(const std::string& k, float fallback) const {
    auto it = metrics.find(k);
    return it == metrics.end() ? fallback : it->second;
}
Color SkinModel::color(const std::string& k, Color fallback) const {
    auto it = palette.find(k);
    return it == palette.end() ? fallback : it->second;
}
std::string SkinModel::label(const std::string& key, const std::string& fallback) const {
    auto it = text.find(key);
    return it == text.end() ? fallback : it->second;
}
bool SkinModel::on_screen(const std::vector<std::string>& screens, const std::string& screen) const {
    if (screens.empty()) return true;
    for (const auto& s : screens)
        if (s == screen || s == "*") return true;
    return false;
}

std::string resolve_asset(const SkinModel& m, const std::string& p) {
    if (p.empty()) return p;
    if (p.rfind("$exe/", 0) == 0) return join_path(current_exe_dir(), p.substr(5));
    if (p.rfind("$skins/", 0) == 0) return join_path(dir_of(m.dir), p.substr(7));
    if (p[0] == '/') return p;
    return join_path(m.dir, p);
}

int64_t skin_mtime(const SkinModel& m) {
    struct stat st{};
    if (stat(join_path(m.dir, "skin.json").c_str(), &st) != 0) return 0;
#if defined(__APPLE__)
    return static_cast<int64_t>(st.st_mtimespec.tv_sec) * 1000000000 + st.st_mtimespec.tv_nsec;
#else
    return static_cast<int64_t>(st.st_mtime);
#endif
}

}  // namespace r4l
