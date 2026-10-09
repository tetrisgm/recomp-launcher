#include "discscan.h"

#include "session.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <dirent.h>
#include <set>
#include <sys/stat.h>

namespace r4l {

namespace {
bool is_dir(const std::string& p) {
    struct stat st{};
    return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}
std::string lower_ext(const std::string& n) {
    const size_t d = n.find_last_of('.');
    std::string e = d == std::string::npos ? "" : n.substr(d + 1);
    for (char& c : e) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return e;
}
// Stops early when `stop` returns true for a found image (first match wins).
bool walk(const std::string& dir, int depth, size_t& budget, std::vector<std::string>& out,
          const std::function<bool(const std::string&)>* stop, std::chrono::steady_clock::time_point deadline) {
    if (depth < 0 || budget == 0 || std::chrono::steady_clock::now() > deadline) return false;
    DIR* d = opendir(dir.c_str());
    if (!d) return false;
    std::vector<std::string> subdirs;
    while (dirent* e = readdir(d)) {
        if (budget == 0) break;
        --budget;
        const std::string n = e->d_name;
        if (n.empty() || n[0] == '.') continue;
        const std::string p = join_path(dir, n);
        if (is_dir(p)) {
            if (n != "node_modules" && n != "Library" && n != "build" && n.find(".app") == std::string::npos)
                subdirs.push_back(p);
            continue;
        }
        const std::string ext = lower_ext(n);
        if (ext == "cue" || ext == "chd" || ext == "iso") {
            out.push_back(p);
            if (stop && (*stop)(p)) {
                closedir(d);
                return true;
            }
        }
    }
    closedir(d);
    std::sort(subdirs.begin(), subdirs.end());
    for (const auto& s : subdirs)
        if (walk(s, depth - 1, budget, out, stop, deadline)) return true;
    return false;
}
}  // namespace

std::vector<std::string> default_scan_roots(const std::string& exe_dir) {
    std::vector<std::string> r{exe_dir, dir_of(exe_dir)};
    const char* home = std::getenv("HOME");
#if defined(_WIN32)
    if (!home) home = std::getenv("USERPROFILE");
#endif
    if (home) {
        const std::string h = home;
        for (const char* sub : {"Games", "games", "Downloads", "Documents", "Desktop", "ROMs", "roms", "Roms",
                                "Emulation/roms/psx", "Emulation/roms", "RetroArch/roms"})
            r.push_back(join_path(h, sub));
    }
    for (const char* media : {"/run/media", "/media", "/Volumes"})
        if (DIR* d = opendir(media)) {  // removable drives (Steam Deck SD card, USB)
            while (dirent* e = readdir(d)) {
                if (e->d_name[0] == '.') continue;
                const std::string p = join_path(media, e->d_name);
#if !defined(_WIN32)
                struct stat lst{};
                // /Volumes/<boot disk> is a symlink to "/": never walk the system disk.
                if (lstat(p.c_str(), &lst) == 0 && S_ISLNK(lst.st_mode)) continue;
#endif
                r.push_back(p);
                // /run/media/<user>/<card>: one level more on Linux.
                if (std::string(media) == "/run/media")
                    if (DIR* u = opendir(p.c_str())) {
                        while (dirent* c = readdir(u))
                            if (c->d_name[0] != '.') r.push_back(join_path(p, c->d_name));
                        closedir(u);
                    }
            }
            closedir(d);
        }
    std::vector<std::string> out;
    std::set<std::string> seen;
    for (const auto& p : r)
        if (is_dir(p) && seen.insert(p).second) out.push_back(p);
    return out;
}

std::vector<std::string> scan_disc_images(const std::vector<std::string>& roots, int max_depth, size_t max_entries) {
    std::vector<std::string> out;
    size_t budget = max_entries;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    for (const auto& r : roots) walk(r, max_depth, budget, out, nullptr, deadline);
    std::set<std::string> seen;
    std::vector<std::string> uniq;
    for (const auto& p : out)
        if (seen.insert(p).second) uniq.push_back(p);
    return uniq;
}

std::string find_disc(const std::vector<std::string>& roots, const std::function<bool(const std::string&)>& accept,
                      int max_depth) {
    // Verify as we go and stop at the first accepted image; bounded in time.
    std::vector<std::string> seen;
    size_t budget = 20000;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    for (const auto& r : roots)
        if (walk(r, max_depth, budget, seen, &accept, deadline)) return seen.back();
    return "";
}

}  // namespace r4l
