// memcard.h — create an empty, formatted PlayStation memory card image.
#pragma once

#include <string>

namespace r4l {

// 128 KiB card: "MC" header frame, 15 free directory frames, an empty broken-
// sector list and the test frame, each with its XOR checksum. Returns false
// when the file cannot be written.
bool format_memcard(const std::string& path);

}  // namespace r4l
