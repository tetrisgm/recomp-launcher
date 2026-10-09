// title_r4.cpp — R4: Ridge Racer Type 4 (USA), SLUS-00797.
#include "r4l/title.h"

#include "imgui.h"

#include <cmath>

namespace r4l {
namespace {

const ControllerProfile kControllers[] = {
    {"dualshock", "DualShock", "Analog sticks and triggers. Best with a modern gamepad.", 1,
     {nullptr, nullptr}},
    {"digital", "Digital pad", "The original PlayStation pad, no sticks.", 2, {nullptr, nullptr}},
    {"negcon", "NeGcon", "Twist steering and analog pedals, mapped from a gamepad.", 1,
     {nullptr, nullptr}},
    {"jogcon", "JogCon", "Namco's force-feedback dial. Uses the R4 JogCon input mod.", 1,
     {"r4.compat.jogcon-input", "jogcon-input"}},
};

const FeatureRef kGraphicsFeatures[] = {
    {"r4.enhancement.widescreen", "widescreen"},
    {"r4.enhancement.max-detail", "max-detail"},
    {"psx.enhancement.pgxp", "pgxp"},
};

const TitleNotice kNotices[] = {
    {"HD HUD (T4HDHUD)",
     "The bundled HD HUD pack is HD HUD by Kuid0us, re-keyed for the US disc. "
     "Credit to Kuid0us.",
     "https://github.com/Kuid0us/T4HDHUD"},
    {"Lato",
     "LatoLatin Regular and Bold. Copyright (c) 2010-2015 Lukasz Dziedzic, with Reserved "
     "Font Name \"Lato\". Licensed under the SIL Open Font License 1.1.",
     "https://scripts.sil.org/OFL"},
    {"Dear ImGui", "Copyright (c) 2014-2026 Omar Cornut. MIT License.",
     "https://github.com/ocornut/imgui"},
    {"psxrecomp", "Static recompilation framework for PlayStation titles. MIT License.",
     "https://github.com/RetroPortingToolKit/psxrecomp"},
    {"Ridge Racer Type 4",
     "R4: Ridge Racer Type 4 is a trademark of Bandai Namco Entertainment. This project "
     "ships no game data; you supply your own disc.",
     nullptr},
};

// Speed lines and a horizon: a cheap, resolution-independent hero.
void draw_hero(void* dl_v, float x0, float y0, float x1, float y1, double t) {
    ImDrawList* dl = static_cast<ImDrawList*>(dl_v);
    const float w = x1 - x0, h = y1 - y0;
    dl->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(18, 10, 40, 255),
                                IM_COL32(60, 12, 58, 255), IM_COL32(230, 90, 40, 255),
                                IM_COL32(20, 20, 60, 255));
    const float hy = y0 + h * 0.62f;
    dl->AddRectFilled(ImVec2(x0, hy), ImVec2(x1, y1), IM_COL32(8, 8, 18, 235));
    const ImVec2 vp(x0 + w * 0.62f, hy);
    for (int i = -9; i <= 9; ++i) {
        const float fx = vp.x + i * w * 0.12f;
        dl->AddLine(vp, ImVec2(fx, y1), IM_COL32(255, 120, 60, 70), 1.0f);
    }
    for (int i = 1; i <= 7; ++i) {
        float f = static_cast<float>(std::fmod(i / 7.0 + t * 0.25, 1.0));
        f = f * f;
        const float yy = hy + (y1 - hy) * f;
        dl->AddLine(ImVec2(x0, yy), ImVec2(x1, yy), IM_COL32(255, 120, 60, (int)(30 + 90 * f)), 1.0f);
    }
    dl->AddCircleFilled(ImVec2(vp.x, hy - h * 0.02f), h * 0.16f, IM_COL32(255, 170, 70, 200), 48);
    dl->AddRectFilled(ImVec2(x0, hy - 2), ImVec2(x1, hy), IM_COL32(255, 200, 120, 160));
}

const TitleLayer kR4 = {
    "r4",
    "R4: Ridge Racer Type 4",
    "Native PC recompilation · SLUS-00797",
    "SLUS-00797",
    {255, 92, 54},
    {120, 92, 255},
    4,
    4,
    {"r4.modern-controls", "modern-controls"},
    "scheme",
    "modern",
    "classic",
    kGraphicsFeatures,
    static_cast<int>(sizeof(kGraphicsFeatures) / sizeof(kGraphicsFeatures[0])),
    kControllers,
    static_cast<int>(sizeof(kControllers) / sizeof(kControllers[0])),
    kNotices,
    static_cast<int>(sizeof(kNotices) / sizeof(kNotices[0])),
    "A static recompilation of R4: Ridge Racer Type 4 (USA) built on psxrecomp. "
    "It runs natively, with no emulator behind it.",
    draw_hero,
};

}  // namespace

const TitleLayer& active_title() { return kR4; }

}  // namespace r4l
