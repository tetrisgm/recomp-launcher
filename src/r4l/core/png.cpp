// png.cpp — minimal PNG writer (stored zlib blocks, no dependency).
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace r4l {

// Minimal PNG writer: zlib "stored" blocks, no compression dependency.
namespace {
uint32_t crc_table[256];
void crc_init() {
    for (uint32_t n = 0; n < 256; ++n) {
        uint32_t c = n;
        for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
}
uint32_t crc(const unsigned char* b, size_t n, uint32_t c = 0xFFFFFFFFu) {
    for (size_t i = 0; i < n; ++i) c = crc_table[(c ^ b[i]) & 0xFF] ^ (c >> 8);
    return c;
}
void be32(std::vector<unsigned char>& v, uint32_t x) {
    v.push_back(x >> 24);
    v.push_back(x >> 16);
    v.push_back(x >> 8);
    v.push_back(x);
}
void chunk(std::vector<unsigned char>& out, const char* type, const std::vector<unsigned char>& data) {
    be32(out, static_cast<uint32_t>(data.size()));
    std::vector<unsigned char> td(type, type + 4);
    td.insert(td.end(), data.begin(), data.end());
    out.insert(out.end(), td.begin(), td.end());
    be32(out, crc(td.data(), td.size()) ^ 0xFFFFFFFFu);
}
}  // namespace

bool write_png_rgba(const std::string& path, int w, int h, const unsigned char* rgba) {
    crc_init();
    std::vector<unsigned char> raw;
    raw.reserve(static_cast<size_t>(h) * (w * 4 + 1));
    for (int y = 0; y < h; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgba + static_cast<size_t>(y) * w * 4, rgba + static_cast<size_t>(y + 1) * w * 4);
    }
    std::vector<unsigned char> z{0x78, 0x01};
    uint32_t a = 1, b = 0;
    for (unsigned char c : raw) {
        a = (a + c) % 65521;
        b = (b + a) % 65521;
    }
    size_t off = 0;
    while (off < raw.size()) {
        const size_t n = std::min<size_t>(65535, raw.size() - off);
        z.push_back(off + n == raw.size() ? 1 : 0);
        z.push_back(n & 0xFF);
        z.push_back(n >> 8);
        z.push_back(~n & 0xFF);
        z.push_back((~n >> 8) & 0xFF);
        z.insert(z.end(), raw.begin() + off, raw.begin() + off + n);
        off += n;
    }
    be32(z, (b << 16) | a);
    std::vector<unsigned char> out{0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::vector<unsigned char> ihdr;
    be32(ihdr, w);
    be32(ihdr, h);
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0});
    chunk(out, "IHDR", ihdr);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", {});
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(f);
}

}  // namespace r4l
