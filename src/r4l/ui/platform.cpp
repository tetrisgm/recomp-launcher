#include "platform.h"

#include "ui.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

namespace r4l {

namespace {
PFNGLGENFRAMEBUFFERSPROC p_glGenFramebuffers;
PFNGLBINDFRAMEBUFFERPROC p_glBindFramebuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC p_glFramebufferTexture2D;
PFNGLDELETEFRAMEBUFFERSPROC p_glDeleteFramebuffers;

bool file_exists(const std::string& p) {
    std::ifstream f(p);
    return static_cast<bool>(f);
}
}  // namespace

bool Platform::open(const char* title, int w, int h, bool hide, const char* icon_path,
                    const std::string& assets_dir) {
    hidden = hide;
    width = w;
    height = h;
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) return false;
        owns_sdl = true;
    } else if (!SDL_WasInit(SDL_INIT_GAMEPAD)) {
        SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (hidden) flags |= SDL_WINDOW_HIDDEN;
    window = SDL_CreateWindow(title, w, h, flags);
    if (!window) return false;
    gl = SDL_GL_CreateContext(window);
    if (!gl) return false;
    SDL_GL_MakeCurrent(window, static_cast<SDL_GLContext>(gl));
    SDL_GL_SetSwapInterval(1);
    if (icon_path && *icon_path) {
        if (SDL_Surface* s = SDL_LoadBMP(icon_path)) {
            SDL_SetWindowIcon(window, s);
            SDL_DestroySurface(s);
        }
    }
    p_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
    p_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
    p_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");
    p_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // launcher state lives in the ABI, not imgui.ini
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigNavCursorVisibleAuto = true;
    ImGui_ImplSDL3_InitForOpenGL(window, gl);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Fonts: Lato from the staged assets, ImGui's default as a fallback.
    Theme& th = const_cast<Theme&>(theme());
    const float scale = SDL_GetWindowDisplayScale(window) > 0 ? 1.0f : 1.0f;
    const std::string reg = assets_dir + "/fonts/LatoLatin-Regular.ttf";
    const std::string bold = assets_dir + "/fonts/LatoLatin-Bold.ttf";
    th.body_size = 18.0f;
    if (file_exists(reg)) th.body = io.Fonts->AddFontFromFileTTF(reg.c_str(), th.body_size);
    if (!th.body) th.body = io.Fonts->AddFontDefault();
    if (file_exists(bold)) th.bold = io.Fonts->AddFontFromFileTTF(bold.c_str(), th.body_size);
    if (!th.bold) th.bold = th.body;
    io.FontDefault = th.body;
    ImGui::GetStyle().FontSizeBase = th.body_size;
    (void)scale;
    return true;
}

bool Platform::pump(App& app) {
    SDL_Event e;
    bool alive = true;
    while (SDL_PollEvent(&e)) {
        if (app.handle_event(e)) continue;
        ImGui_ImplSDL3_ProcessEvent(&e);
        if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) alive = false;
        if (e.type == SDL_EVENT_GAMEPAD_ADDED) SDL_OpenGamepad(e.gdevice.which);
    }
    return alive;
}

void Platform::begin_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    if (hidden) {
        // A hidden window can report 0x0; render at the requested size.
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
        io.DisplayFramebufferScale = ImVec2(1, 1);
    }
    ImGui::NewFrame();
}

void Platform::end_frame(bool to_fbo) {
    ImGui::Render();
    int fw = width, fh = height;
    if (to_fbo && p_glGenFramebuffers) {
        if (!fbo) {
            glGenTextures(1, &fbo_tex);
            glBindTexture(GL_TEXTURE_2D, fbo_tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            p_glGenFramebuffers(1, &fbo);
            p_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
        }
        p_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    } else {
        SDL_GetWindowSizeInPixels(window, &fw, &fh);
    }
    glViewport(0, 0, fw, fh);
    const ImVec4 bg = theme().bg;
    glClearColor(bg.x, bg.y, bg.z, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (to_fbo && p_glBindFramebuffer) {
        glFinish();
    } else {
        SDL_GL_SwapWindow(window);
    }
}

bool Platform::capture_png(const std::string& path) {
    if (!fbo) return false;
    std::vector<unsigned char> px(static_cast<size_t>(width) * height * 4);
    p_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // GL rows are bottom-up.
    std::vector<unsigned char> flipped(px.size());
    const size_t row = static_cast<size_t>(width) * 4;
    for (int y = 0; y < height; ++y)
        std::memcpy(&flipped[y * row], &px[(height - 1 - y) * row], row);
    return write_png_rgba(path, width, height, flipped.data());
}

void Platform::close(bool preserve_sdl) {
    if (fbo && p_glDeleteFramebuffers) p_glDeleteFramebuffers(1, &fbo);
    if (fbo_tex) glDeleteTextures(1, &fbo_tex);
    fbo = fbo_tex = 0;
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    if (gl) SDL_GL_DestroyContext(static_cast<SDL_GLContext>(gl));
    if (window) SDL_DestroyWindow(window);
    gl = nullptr;
    window = nullptr;
    if (owns_sdl && !preserve_sdl) SDL_Quit();
}

}  // namespace r4l
