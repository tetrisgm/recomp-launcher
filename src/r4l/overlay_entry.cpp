// overlay_entry.cpp — recomp_launcher_overlay.h: the launcher's menus over a
// running game, on the runtime's own window and GL context.
#include "recomp_launcher_overlay.h"

#include "r4l/ui/platform.h"
#include "r4l/ui/skin.h"
#include "r4l/ui/ui.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

namespace {
using namespace r4l;

struct Overlay {
    std::unique_ptr<App> app;
    ImGuiContext* ctx = nullptr;
    SDL_Window* window = nullptr;
    const RecompOverlayHost* host = nullptr;
    bool open = false;
    bool paused = false;
    unsigned pad_held = 0;
    unsigned combo = (1u << SDL_GAMEPAD_BUTTON_START) | (1u << SDL_GAMEPAD_BUTTON_BACK);
};
Overlay* g_ov = nullptr;

void set_open(bool open) {
    Overlay& o = *g_ov;
    if (open == o.open) return;
    o.open = open;
    App& a = *o.app;
    const bool netplay = o.host && o.host->netplay_active && o.host->netplay_active(o.host->ctx);
    a.netplay_locked = netplay;
    if (open) {
        a.overlay_open = true;
        a.screen = Screen::Graphics;
        a.focus_request = Screen::Graphics;
        if (a.s.io) a.opened = a.applied = *a.s.io;
        o.paused = !netplay && o.host && o.host->set_paused && o.host->set_paused(o.host->ctx, 1);
        skin().play("confirm");
    } else {
        std::string err;
        a.s.commit_files(&err);
        if (o.paused && o.host && o.host->set_paused) o.host->set_paused(o.host->ctx, 0);
        o.paused = false;
        skin().play("back");
    }
}
}  // namespace

extern "C" int recomp_overlay_init(void* sdl_window, void* gl_context, const RecompLauncherCGameInfo* game,
                                   RecompLauncherCSettings* io, const RecompOverlayHost* host,
                                   const char* assets_dir) {
    if (g_ov) return 1;
    g_ov = new Overlay();
    g_ov->window = static_cast<SDL_Window*>(sdl_window);
    g_ov->host = host;
    g_ov->app = std::make_unique<App>();
    App& a = *g_ov->app;
    a.mode = Mode::Overlay;
    a.host = host;
    a.begin(io, game, resolve_assets_dir(assets_dir).c_str(), nullptr);
    if (host && host->mod_set_live)
        a.mods.live = [host](const char* p, const char* f, const char* o, const char* v) {
            return host->mod_set_live(host->ctx, p, f, o, v);
        };
    if (const char* c = std::getenv("R4L_OVERLAY_COMBO")) g_ov->combo = static_cast<unsigned>(std::strtoul(c, nullptr, 0));

    ImGuiContext* prev = ImGui::GetCurrentContext();
    g_ov->ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(g_ov->ctx);
    ImGuiIO& im = ImGui::GetIO();
    im.IniFilename = nullptr;
    im.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplSDL3_InitForOpenGL(g_ov->window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");
    Theme& th = const_cast<Theme&>(theme());
    th.body = im.Fonts->AddFontDefault();
    th.bold = th.body;
    apply_theme(*a.title, 1.0f);
    a.init_skin();
    if (prev) ImGui::SetCurrentContext(prev);
    return 1;
}

extern "C" int recomp_overlay_handle_event(const void* ev) {
    if (!g_ov || !ev) return 0;
    const SDL_Event& e = *static_cast<const SDL_Event*>(ev);
    Overlay& o = *g_ov;
    bool toggle = false;
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE && !e.key.repeat &&
        (!o.open || o.app->capture.kind == Capture::None))
        toggle = !o.open;  // Esc opens; inside the menu Esc is "back" (handled by the frame)
    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        o.pad_held |= 1u << e.gbutton.button;
        if (e.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE) toggle = true;
        if ((o.pad_held & o.combo) == o.combo) toggle = true;
    }
    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_UP) o.pad_held &= ~(1u << e.gbutton.button);
    if (toggle) {
        set_open(!o.open);
        return 1;
    }
    if (!o.open) return 0;
    ImGuiContext* prev = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(o.ctx);
    if (!o.app->handle_event(e)) ImGui_ImplSDL3_ProcessEvent(&e);
    ImGui::SetCurrentContext(prev);
    return 1;  // the game sees nothing while the menu is open
}

extern "C" int recomp_overlay_is_open(void) { return g_ov && g_ov->open; }
extern "C" void recomp_overlay_set_open(int open) {
    if (g_ov) set_open(open != 0);
}

extern "C" void recomp_overlay_render(int fb_w, int fb_h) {
    if (!g_ov || !g_ov->open) return;
    Overlay& o = *g_ov;
    ImGuiContext* prev = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(o.ctx);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(fb_w), static_cast<float>(fb_h));
    io.DisplayFramebufferScale = ImVec2(1, 1);
    ImGui::NewFrame();
    o.app->frame();
    ImGui::Render();
    glViewport(0, 0, fb_w, fb_h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::SetCurrentContext(prev);
    if (!o.app->overlay_open) set_open(false);
    o.app->overlay_open = o.open;
}

extern "C" void recomp_overlay_shutdown(void) {
    if (!g_ov) return;
    ImGuiContext* prev = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(g_ov->ctx);
    skin().unload();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(g_ov->ctx);
    ImGui::SetCurrentContext(prev == g_ov->ctx ? nullptr : prev);
    delete g_ov;
    g_ov = nullptr;
}

// Test/screenshot helper (not ABI): renders a stand-in game frame with the
// overlay open, into a hidden window's FBO, and writes a PNG.
extern "C" int r4l_render_overlay_png(const char* path, int w, int h, const RecompLauncherCGameInfo* game,
                                      RecompLauncherCSettings* io, const RecompOverlayHost* host,
                                      const char* assets_dir, const char* screen) {
    Platform plat;
    if (!plat.open("overlay", w, h, true, nullptr, assets_dir ? assets_dir : "assets")) return 0;
    // Platform::open made its own ImGui context; the overlay brings its own.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    recomp_overlay_init(plat.window, plat.gl, game, io, host, assets_dir);
    recomp_overlay_set_open(1);
    if (screen)
        for (int i = 0; i < static_cast<int>(Screen::Count); ++i)
            if (!std::strcmp(screen, screen_name(static_cast<Screen>(i)))) g_ov->app->screen = static_cast<Screen>(i);
    // Bind an FBO and paint a fake game frame (sky, road, horizon bands).
    plat.width = w;
    plat.height = h;
    for (int frame = 0; frame < 4; ++frame) {
        ImGuiContext* none = nullptr;
        ImGui::SetCurrentContext(none);
        // Reuse Platform's FBO path by rendering an empty ImGui-less clear.
        auto fbo_gen = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
        auto fbo_bind = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
        auto fbo_tex = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");
        if (!plat.fbo) {
            glGenTextures(1, &plat.fbo_tex);
            glBindTexture(GL_TEXTURE_2D, plat.fbo_tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            fbo_gen(1, &plat.fbo);
            fbo_bind(GL_FRAMEBUFFER, plat.fbo);
            fbo_tex(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, plat.fbo_tex, 0);
        }
        fbo_bind(GL_FRAMEBUFFER, plat.fbo);
        glViewport(0, 0, w, h);
        glEnable(GL_SCISSOR_TEST);
        const float bands[][4] = {{0.20f, 0.42f, 0.78f, 1}, {0.95f, 0.62f, 0.35f, 1}, {0.16f, 0.30f, 0.14f, 1}, {0.22f, 0.22f, 0.24f, 1}};
        for (int b = 0; b < 4; ++b) {
            glScissor(0, h - (b + 1) * h / 4, w, h / 4);
            glClearColor(bands[b][0], bands[b][1], bands[b][2], 1);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        glScissor(w * 2 / 5, 0, w / 5, h / 4);
        glClearColor(0.9f, 0.9f, 0.9f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        recomp_overlay_render(w, h);
        glFinish();
    }
    const bool ok = plat.capture_png(path);
    recomp_overlay_shutdown();
    plat.close(false);
    return ok;
}
