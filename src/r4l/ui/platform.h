// platform.h — SDL3 window + OpenGL 3.3 core + Dear ImGui backends.
#pragma once

#include <string>

struct SDL_Window;

namespace r4l {

struct App;

struct Platform {
    SDL_Window* window = nullptr;
    void* gl = nullptr;
    bool owns_sdl = false;
    bool hidden = false;
    int width = 1280, height = 800;
    unsigned fbo = 0, fbo_tex = 0;

    bool open(const char* title, int w, int h, bool hidden, const char* icon_path,
              const std::string& assets_dir);
    // Pump events into ImGui and the app. Returns false when the window closes.
    bool pump(App& app);
    void begin_frame();
    void end_frame(bool to_fbo);
    bool capture_png(const std::string& path);  // reads the FBO
    void close(bool preserve_sdl);
};

bool write_png_rgba(const std::string& path, int w, int h, const unsigned char* rgba);

}  // namespace r4l
