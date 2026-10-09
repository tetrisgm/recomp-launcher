// discscan.h — find the player's disc image without asking.
//
// Walks a few well-known folders (the game's own folder, Games, Downloads,
// Documents, Desktop, ROM folders, EmuDeck's Emulation/roms/psx, removable
// media under /run/media and /Volumes) a few levels deep, collects .cue/.chd/
// .iso files, and lets the caller keep the first one its disc_verify accepts
// (the title's serial, a good dump). Bounded: depth and file count limits.
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace r4l {

std::vector<std::string> default_scan_roots(const std::string& exe_dir);
std::vector<std::string> scan_disc_images(const std::vector<std::string>& roots, int max_depth = 4,
                                          size_t max_entries = 20000);
// First candidate `accept` returns true for, or "".
std::string find_disc(const std::vector<std::string>& roots,
                      const std::function<bool(const std::string&)>& accept, int max_depth = 4);

}  // namespace r4l
