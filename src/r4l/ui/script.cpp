#include "script.h"

#include "platform.h"
#include "ui.h"

#include "r4l/core/ini.h"

#include <algorithm>
#include <cstdlib>

#include <SDL3/SDL.h>

#include <cstdio>
#include <fstream>
#include <sstream>

namespace r4l {

bool Script::load(const std::string& path) {
    std::ifstream f(path);
    std::string l;
    while (std::getline(f, l)) {
        l = trim(l);
        if (!l.empty() && l[0] != '#') lines_.push_back(l);
    }
    return !lines_.empty();
}

void Script::step(App& app, Platform& plat) {
    if (wait_ > 0) {
        --wait_;
        return;
    }
    const Uint32 win = plat.window ? SDL_GetWindowID(plat.window) : 0;
    while (pc_ < lines_.size()) {
        const std::string line = lines_[pc_++];
        std::istringstream in(line);
        std::string op;
        in >> op;
        std::string rest;
        std::getline(in, rest);
        rest = trim(rest);
        std::fprintf(stderr, "[r4l-script] %s\n", line.c_str());
        SDL_Event e;
        if (op == "wait") {
            wait_ = std::max(1, std::atoi(rest.c_str()));
            return;
        } else if (op == "screen") {
            for (int i = 0; i < static_cast<int>(Screen::Count); ++i)
                if (rest == screen_name(static_cast<Screen>(i))) app.screen = static_cast<Screen>(i);
        } else if (op == "click" || op == "wheel") {
            float x = 0, y = 0;
            int steps = 0;
            std::istringstream a(rest);
            a >> x >> y >> steps;
            SDL_zero(e);
            e.type = SDL_EVENT_MOUSE_MOTION;
            e.motion.windowID = win;
            e.motion.x = x;
            e.motion.y = y;
            SDL_PushEvent(&e);
            if (op == "wheel") {
                SDL_zero(e);
                e.type = SDL_EVENT_MOUSE_WHEEL;
                e.wheel.windowID = win;
                e.wheel.y = static_cast<float>(steps);
                e.wheel.mouse_x = x;
                e.wheel.mouse_y = y;
                SDL_PushEvent(&e);
            } else {
                for (int down = 1; down >= 0; --down) {
                    SDL_zero(e);
                    e.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
                    e.button.windowID = win;
                    e.button.button = SDL_BUTTON_LEFT;
                    e.button.down = down != 0;
                    e.button.clicks = 1;
                    e.button.x = x;
                    e.button.y = y;
                    SDL_PushEvent(&e);
                }
            }
            wait_ = 2;  // let ImGui see down and up on separate frames
            return;
        } else if (op == "key") {
            const SDL_Scancode sc = SDL_GetScancodeFromName(rest.c_str());
            for (int down = 1; down >= 0; --down) {
                SDL_zero(e);
                e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
                e.key.windowID = win;
                e.key.scancode = sc;
                e.key.key = SDL_GetKeyFromScancode(sc, SDL_KMOD_NONE, false);
                e.key.down = down != 0;
                SDL_PushEvent(&e);
            }
            wait_ = 2;
            return;
        } else if (op == "text") {
            SDL_zero(e);
            e.type = SDL_EVENT_TEXT_INPUT;
            e.text.windowID = win;
            static std::string keep;
            keep = rest;
            e.text.text = keep.c_str();
            SDL_PushEvent(&e);
            wait_ = 2;
            return;
        } else if (op == "shot") {
            plat.capture_png(rest);
        } else if (op == "status") {
            const bool ok = app.s.status.find(rest) != std::string::npos || app.np_status.find(rest) != std::string::npos;
            std::fprintf(stderr, "[r4l-script] %s status \"%s\" (now \"%s\")\n", ok ? "PASS" : "FAIL", rest.c_str(),
                         app.s.status.c_str());
            if (!ok) ++failures_;
        } else if (op == "play") {
            app.request_launch();
        } else if (op == "quit") {
            app.request_quit();
        }
    }
}

}  // namespace r4l
