// discfs.h — read files from the player's own PlayStation disc image, and
// decode PlayStation TIM images, for disc-sourced skin assets. Nothing read
// here is ever shipped: it is converted into a local cache next to the game.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace r4l {

class DiscImage {
public:
    // .cue (first data track's BIN), raw .bin (2352-byte sectors) or .iso
    // (2048-byte sectors). CHD is not supported (returns false).
    bool open(const std::string& path, std::string* err);
    // ISO9660 path, e.g. "R4.BIN" or "DATA/FILE.DAT" (";1" optional).
    bool read_file(const std::string& iso_path, std::vector<uint8_t>* out, std::string* err);

private:
    bool sector(uint32_t lba, uint8_t out[2048]);
    std::string bin_;
    int sector_size_ = 2352;
    int data_offset_ = 24;
};

struct Rgba {
    int w = 0, h = 0;
    std::vector<uint8_t> px;  // RGBA8
};

// Decode the TIM that starts at `offset` in `data` (4/8/16-bit, with CLUT).
// Colour 0x0000 is transparent, as on the console.
bool decode_tim(const std::vector<uint8_t>& data, size_t offset, Rgba* out);
Rgba crop(const Rgba& img, int x, int y, int w, int h);
// Recolour: every pixel keeps its alpha and luminance-scaled RGB of `rgb`.
void tint(Rgba* img, uint8_t r, uint8_t g, uint8_t b);
Rgba scale_nearest(const Rgba& img, int factor);

// Proportional bitmap font from an atlas: each row band [y0,y1) holds the
// characters of `chars` left to right, separated by fully transparent
// columns. Writes <name>.png and <name>.fnt (BMFont text) into dir.
bool write_bitmap_font(const Rgba& atlas, const std::vector<std::pair<std::pair<int, int>, std::string>>& rows,
                       const std::string& dir, const std::string& name, int size, std::string* err);

bool write_png_rgba(const std::string& path, int w, int h, const unsigned char* rgba);

}  // namespace r4l
