// script.h — R4L_SCRIPT: drive the launcher from a text file (tests, the
// recomp-ui LNG_SCRIPT equivalent). One action per line:
//   screen <Name> | click <x> <y> | wheel <x> <y> <steps> | key <Scancode> |
//   text <utf8> | wait <frames> | shot <png> | status <substring> | play | quit
// Coordinates are window points. `status` prints PASS/FAIL against the footer
// status line. Results go to stderr as "[r4l-script] ...".
#pragma once

#include <string>
#include <vector>

struct SDL_Window;

namespace r4l {

struct App;
struct Platform;

class Script {
public:
    bool load(const std::string& path);
    bool active() const { return !lines_.empty() && pc_ < lines_.size(); }
    // Run actions until a wait; returns true when the script asked to stop.
    void step(App& app, Platform& plat);
    int failures() const { return failures_; }

private:
    std::vector<std::string> lines_;
    size_t pc_ = 0;
    int wait_ = 0;
    int failures_ = 0;
};

}  // namespace r4l
