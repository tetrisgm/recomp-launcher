#include "memcard.h"

#include <cstring>
#include <fstream>
#include <vector>

namespace r4l {

bool format_memcard(const std::string& path) {
    std::vector<unsigned char> m(131072, 0);
    auto frame = [&](int i) { return &m[static_cast<size_t>(i) * 128]; };
    auto checksum = [&](unsigned char* f) {
        unsigned char x = 0;
        for (int i = 0; i < 127; ++i) x ^= f[i];
        f[127] = x;
    };
    frame(0)[0] = 'M';
    frame(0)[1] = 'C';
    checksum(frame(0));
    for (int i = 1; i <= 15; ++i) {
        unsigned char* f = frame(i);
        f[0] = 0xA0;  // free block
        f[8] = f[9] = 0xFF;
        checksum(f);
    }
    for (int i = 16; i <= 35; ++i) {
        unsigned char* f = frame(i);
        std::memset(f, 0, 128);
        f[0] = f[1] = f[2] = f[3] = 0xFF;
        f[8] = f[9] = 0xFF;
        checksum(f);
    }
    std::memcpy(frame(63), frame(0), 128);
    std::ofstream o(path, std::ios::binary | std::ios::trunc);
    o.write(reinterpret_cast<const char*>(m.data()), static_cast<std::streamsize>(m.size()));
    return static_cast<bool>(o);
}

}  // namespace r4l
