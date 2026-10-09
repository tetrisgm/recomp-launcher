#include "discfs.h"

#include "ini.h"
#include "session.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>

namespace r4l {

namespace {
std::string upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}
uint32_t le32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<uint32_t>(p[3]) << 24); }
uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
}  // namespace

bool DiscImage::open(const std::string& path, std::string* err) {
    std::string lower = path;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    auto ends = [&](const char* e) {
        const size_t n = std::strlen(e);
        return lower.size() >= n && lower.compare(lower.size() - n, n, e) == 0;
    };
    if (ends(".chd")) {
        if (err) *err = "CHD images are not read for skin assets";
        return false;
    }
    bin_ = path;
    sector_size_ = 2352;
    data_offset_ = 24;
    if (ends(".cue")) {
        std::ifstream f(path);
        std::string line;
        bin_.clear();
        while (std::getline(f, line)) {
            const std::string t = trim(line);
            if (upper(t).rfind("FILE", 0) == 0) {
                const size_t a = t.find('"'), b = t.rfind('"');
                if (a != std::string::npos && b > a) {
                    bin_ = join_path(dir_of(path), t.substr(a + 1, b - a - 1));
                    break;
                }
            }
        }
        if (bin_.empty()) {
            if (err) *err = "No FILE line in the cue sheet";
            return false;
        }
    } else if (ends(".iso")) {
        sector_size_ = 2048;
        data_offset_ = 0;
    }
    uint8_t pvd[2048];
    if (!sector(16, pvd) || std::memcmp(pvd + 1, "CD001", 5) != 0) {
        // MODE1/2352 keeps data at offset 16.
        data_offset_ = 16;
        if (sector_size_ != 2352 || !sector(16, pvd) || std::memcmp(pvd + 1, "CD001", 5) != 0) {
            if (err) *err = "Not an ISO9660 PlayStation disc";
            return false;
        }
    }
    return true;
}

bool DiscImage::sector(uint32_t lba, uint8_t out[2048]) {
    std::ifstream f(bin_, std::ios::binary);
    if (!f) return false;
    f.seekg(static_cast<std::streamoff>(lba) * sector_size_ + data_offset_);
    f.read(reinterpret_cast<char*>(out), 2048);
    return static_cast<bool>(f);
}

bool DiscImage::read_file(const std::string& iso_path, std::vector<uint8_t>* out, std::string* err) {
    uint8_t pvd[2048];
    if (!sector(16, pvd)) {
        if (err) *err = "Cannot read the disc";
        return false;
    }
    uint32_t lba = le32(pvd + 156 + 2), size = le32(pvd + 156 + 10);
    std::vector<std::string> parts;
    std::stringstream ss(upper(iso_path));
    std::string part;
    while (std::getline(ss, part, '/'))
        if (!part.empty()) parts.push_back(part);
    for (size_t pi = 0; pi < parts.size(); ++pi) {
        const bool last = pi + 1 == parts.size();
        bool found = false;
        for (uint32_t s = 0; s < (size + 2047) / 2048 && !found; ++s) {
            uint8_t buf[2048];
            if (!sector(lba + s, buf)) break;
            for (size_t i = 0; i < 2048;) {
                const uint8_t len = buf[i];
                if (!len) break;
                std::string name(reinterpret_cast<char*>(buf + i + 33), buf[i + 32]);
                const size_t semi = name.find(';');
                if (semi != std::string::npos) name.resize(semi);
                if (upper(name) == parts[pi] && (((buf[i + 25] & 2) != 0) != last)) {
                    lba = le32(buf + i + 2);
                    size = le32(buf + i + 10);
                    found = true;
                    break;
                }
                i += len;
            }
        }
        if (!found) {
            if (err) *err = "No " + iso_path + " on this disc";
            return false;
        }
    }
    out->resize(size);
    for (uint32_t s = 0; s * 2048 < size; ++s) {
        uint8_t buf[2048];
        if (!sector(lba + s, buf)) {
            if (err) *err = "Short read";
            return false;
        }
        std::memcpy(out->data() + s * 2048, buf, std::min<size_t>(2048, size - s * 2048));
    }
    return true;
}

namespace {
void c15(uint16_t v, uint8_t* o) {
    o[0] = static_cast<uint8_t>((v & 31) << 3);
    o[1] = static_cast<uint8_t>(((v >> 5) & 31) << 3);
    o[2] = static_cast<uint8_t>(((v >> 10) & 31) << 3);
    o[3] = v == 0 ? 0 : 255;
}
}  // namespace

bool decode_tim(const std::vector<uint8_t>& d, size_t o, Rgba* out) {
    if (o + 8 > d.size() || le32(&d[o]) != 0x10) return false;
    const uint32_t fl = le32(&d[o + 4]);
    if ((fl & ~0xBu) || (fl & 3) == 3) return false;
    size_t p = o + 8;
    std::vector<uint16_t> clut;
    if (fl & 8) {
        if (p + 12 > d.size()) return false;
        const uint32_t bl = le32(&d[p]);
        const uint16_t w = le16(&d[p + 8]), h = le16(&d[p + 10]);
        if (bl != 12u + w * h * 2u || p + bl > d.size()) return false;
        for (int i = 0; i < w * h; ++i) clut.push_back(le16(&d[p + 12 + i * 2]));
        p += bl;
    }
    if (p + 12 > d.size()) return false;
    const uint32_t bl = le32(&d[p]);
    const uint16_t w = le16(&d[p + 8]), h = le16(&d[p + 10]);
    if (!w || !h || bl != 12u + w * h * 2u || p + bl > d.size()) return false;
    const uint8_t* px = &d[p + 12];
    const int bpp = fl & 3;
    out->w = bpp == 0 ? w * 4 : bpp == 1 ? w * 2 : w;
    out->h = h;
    out->px.assign(static_cast<size_t>(out->w) * h * 4, 0);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < out->w; ++x) {
            uint16_t v;
            if (bpp == 0) {
                const uint8_t b = px[y * w * 2 + x / 2];
                const int idx = (x & 1) ? (b >> 4) : (b & 15);
                v = idx < static_cast<int>(clut.size()) ? clut[idx] : 0;
            } else if (bpp == 1) {
                const uint8_t idx = px[y * w * 2 + x];
                v = idx < clut.size() ? clut[idx] : 0;
            } else {
                v = le16(px + (y * w + x) * 2);
            }
            c15(v, &out->px[(static_cast<size_t>(y) * out->w + x) * 4]);
        }
    return true;
}

Rgba crop(const Rgba& img, int x, int y, int w, int h) {
    Rgba o;
    x = std::max(0, x);
    y = std::max(0, y);
    w = std::min(w, img.w - x);
    h = std::min(h, img.h - y);
    if (w <= 0 || h <= 0) return o;
    o.w = w;
    o.h = h;
    o.px.resize(static_cast<size_t>(w) * h * 4);
    for (int r = 0; r < h; ++r)
        std::memcpy(&o.px[static_cast<size_t>(r) * w * 4], &img.px[(static_cast<size_t>(y + r) * img.w + x) * 4], w * 4);
    return o;
}

void tint(Rgba* img, uint8_t r, uint8_t g, uint8_t b) {
    for (size_t i = 0; i + 3 < img->px.size(); i += 4) {
        if (!img->px[i + 3]) continue;
        img->px[i] = r;
        img->px[i + 1] = g;
        img->px[i + 2] = b;
    }
}

Rgba scale_nearest(const Rgba& img, int f) {
    Rgba o;
    o.w = img.w * f;
    o.h = img.h * f;
    o.px.resize(static_cast<size_t>(o.w) * o.h * 4);
    for (int y = 0; y < o.h; ++y)
        for (int x = 0; x < o.w; ++x)
            std::memcpy(&o.px[(static_cast<size_t>(y) * o.w + x) * 4], &img.px[(static_cast<size_t>(y / f) * img.w + x / f) * 4], 4);
    return o;
}

bool write_bitmap_font(const Rgba& atlas, const std::vector<std::pair<std::pair<int, int>, std::string>>& rows,
                       const std::string& dir, const std::string& name, int size, std::string* err) {
    std::ostringstream fnt;
    const int line_h = rows.empty() ? size : rows[0].first.second - rows[0].first.first;
    fnt << "info face=\"" << name << "\" size=" << size << "\n";
    fnt << "common lineHeight=" << line_h << " base=" << line_h - 2 << " scaleW=" << atlas.w << " scaleH=" << atlas.h
        << " pages=1\n";
    fnt << "page id=0 file=\"" << name << ".png\"\n";
    int glyphs = 0;
    for (const auto& row : rows) {
        const int y0 = row.first.first, y1 = row.first.second;
        auto col_empty = [&](int x) {
            for (int y = y0; y < y1 && y < atlas.h; ++y)
                if (atlas.px[(static_cast<size_t>(y) * atlas.w + x) * 4 + 3]) return false;
            return true;
        };
        size_t ci = 0;
        int x = 0;
        while (x < atlas.w && ci < row.second.size()) {
            while (x < atlas.w && col_empty(x)) ++x;
            const int start = x;
            while (x < atlas.w && !col_empty(x)) ++x;
            if (x <= start) break;
            const unsigned char ch = static_cast<unsigned char>(row.second[ci++]);
            if (ch == ' ') continue;
            fnt << "char id=" << static_cast<int>(ch) << " x=" << start << " y=" << y0 << " width=" << (x - start)
                << " height=" << (y1 - y0) << " xoffset=0 yoffset=0 xadvance=" << (x - start + 1) << "\n";
            ++glyphs;
        }
    }
    fnt << "char id=32 x=0 y=0 width=0 height=0 xoffset=0 yoffset=0 xadvance=" << std::max(3, size / 3) << "\n";
    if (glyphs < 10) {
        if (err) *err = "font atlas did not segment";
        return false;
    }
    std::ofstream(join_path(dir, name + ".fnt")) << fnt.str();
    return write_png_rgba(join_path(dir, name + ".png"), atlas.w, atlas.h, atlas.px.data());
}

}  // namespace r4l
