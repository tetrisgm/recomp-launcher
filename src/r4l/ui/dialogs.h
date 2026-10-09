// dialogs.h — file dialogs with a test override.
#pragma once

#include <SDL3/SDL.h>

#include <cstdlib>

namespace r4l {

// Native file dialogs, or the path in R4L_PICK (tests: no dialog is shown and
// the callback gets that path at once).
inline void show_open_file(SDL_DialogFileCallback cb, void* ud, SDL_Window* w, const SDL_DialogFileFilter* f, int n,
                           const char* loc, bool many) {
    if (const char* p = std::getenv("R4L_PICK")) {
        const char* files[2] = {p, nullptr};
        cb(ud, files, -1);
        return;
    }
    SDL_ShowOpenFileDialog(cb, ud, w, f, n, loc, many);
}
inline void show_save_file(SDL_DialogFileCallback cb, void* ud, SDL_Window* w, const SDL_DialogFileFilter* f, int n,
                           const char* loc) {
    if (const char* p = std::getenv("R4L_PICK")) {
        const char* files[2] = {p, nullptr};
        cb(ud, files, -1);
        return;
    }
    SDL_ShowSaveFileDialog(cb, ud, w, f, n, loc);
}
inline void show_open_folder(SDL_DialogFileCallback cb, void* ud, SDL_Window* w, const char* loc, bool many) {
    if (const char* p = std::getenv("R4L_PICK")) {
        const char* files[2] = {p, nullptr};
        cb(ud, files, -1);
        return;
    }
    SDL_ShowOpenFolderDialog(cb, ud, w, loc, many);
}

}  // namespace r4l
