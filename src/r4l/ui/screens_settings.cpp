// screens_settings.cpp — Graphics, Mods and Controls.
#include "ui.h"

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace r4l {

void draw_skin_picker(App& app);

namespace {

void title_text(const char* t, const char* sub) { screen_title(t, sub); }

// Option row for a mod feature option (choice / boolean / integer / text).
void mod_option_row(ModCatalog& mods, ModFeature& f, ModOption& o) {
    ImGui::BeginDisabled(o.info.disabled || !f.info.enabled);
    const char* help = o.info.description[0] ? o.info.description : nullptr;
    if (o.info.type == RECOMP_MOD_OPTION_CHOICE && !o.choices.empty()) {
        std::vector<const char*> labels;
        int cur = -1;
        for (size_t i = 0; i < o.choices.size(); ++i) {
            labels.push_back(o.choices[i].label[0] ? o.choices[i].label : o.choices[i].value);
            if (!std::strcmp(o.choices[i].value, o.info.value)) cur = static_cast<int>(i);
        }
        if (row_combo(o.info.label, &cur, labels.data(), static_cast<int>(labels.size()), help))
            mods.set_option(f, o, o.choices[cur].value);
    } else if (o.info.type == RECOMP_MOD_OPTION_BOOLEAN) {
        int v = !std::strcmp(o.info.value, "true") ? 1 : 0;
        if (row_toggle(o.info.label, &v, help)) mods.set_option(f, o, v ? "true" : "false");
    } else if (o.info.type == RECOMP_MOD_OPTION_INTEGER) {
        int v = std::atoi(o.info.value);
        if (row_slider(o.info.label, &v, static_cast<int>(o.info.min_value), static_cast<int>(o.info.max_value),
                       "%d", help))
            mods.set_option(f, o, std::to_string(v));
    } else {
        row_text(o.info.label, o.info.value, theme().text_dim);
    }
    ImGui::EndDisabled();
}

}  // namespace

// ---------------------------------------------------------------- Graphics

void App::draw_graphics() {
    RecompLauncherCSettings* io = s.io;
    const RecompLauncherCGameInfo* g = s.game;
    title_text("Graphics", "Pick a preset, or change any row. Changing a row a preset sets makes it Custom.");
    if (!io || !g) return;

    if (g->quality_offered_mask) {
        section("Preset");
        ImGui::BeginChild("##preset", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
        const float bw = (ImGui::GetContentRegionAvail().x - 3 * 10) / 4.0f;
        for (int p = kQualityLow; p <= kQualityUltra; ++p) {
            if (p > kQualityLow) ImGui::SameLine(0, 10);
            const bool offered = (g->quality_offered_mask >> (p - 1)) & 1;
            const bool sel = io->quality_preset == p;
            const bool base = io->quality_preset == kQualityCustom && io->quality_base == p;
            std::string label = quality_name(p);
            if (g->quality_detected == p) label += "  (detected)";
            ImGui::BeginDisabled(!offered);
            ImGui::PushID(p);
            if (big_button(label.c_str(), ImVec2(bw, 52), sel)) s.quality.select(p, io);
            if (base) {
                const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddRect(a, b, ImGui::ColorConvertFloat4ToU32(theme().accent), 8.0f, 0, 2.0f);
            }
            ImGui::PopID();
            ImGui::EndDisabled();
        }
        if (io->quality_preset == kQualityCustom) {
            chip("Custom", theme().accent);
            ImGui::SameLine();
            ImGui::TextDisabled("started from %s", quality_name(io->quality_base));
        } else if (io->quality_preset == kQualityUnset) {
            chip("No preset", theme().text_dim);
        }
        ImGui::TextDisabled("%s", g->quality_summary ? g->quality_summary : "Hardware not detected");
        if (g->quality_reason && *g->quality_reason) ImGui::TextDisabled("%s", g->quality_reason);
        if (g->quality_redetect && ImGui::Button("Re-detect")) {
            const int p = g->quality_redetect();
            if (p >= kQualityLow && p <= kQualityUltra) s.quality.select(p, io);
        }
        ImGui::EndChild();
    }

    section("Display");
    if (g->has_renderer && g->num_renderers > 0 && g->renderer_labels)
        row_combo("Renderer", &io->renderer, g->renderer_labels, g->num_renderers, g->renderer_note);
    restart_chip(offsetof(RecompLauncherCSettings, renderer), sizeof(int));
    if (g->has_fullscreen_toggle || true) row_toggle("Fullscreen", &io->fullscreen);
    if (g->has_vsync) {
        static const char* kVsync[] = {"On", "Off (lowest latency)", "Adaptive"};
        int v = io->vsync >= 1 && io->vsync <= 3 ? io->vsync - 1 : 0;
        if (row_combo("V-Sync", &v, kVsync, 3)) io->vsync = v + 1;
    }

    section("Resolution");
    if (g->num_internal_resolutions > 0 && g->internal_resolution_labels && g->internal_resolution_values) {
        int idx = 0;
        for (int i = 0; i < g->num_internal_resolutions; ++i)
            if (g->internal_resolution_values[i] == io->internal_resolution) idx = i;
        if (row_combo("Internal resolution", &idx, g->internal_resolution_labels, g->num_internal_resolutions,
                      g->internal_resolution_note))
            io->internal_resolution = g->internal_resolution_values[idx];
        restart_chip(offsetof(RecompLauncherCSettings, internal_resolution), sizeof(int));
    } else if (g->has_supersampling) {
        row_slider("Supersampling", &io->supersampling, 1, 8, "%dx");
    }
    if (g->has_dynamic_resolution)
        row_toggle("Dynamic resolution", &io->dynamic_resolution,
                   "Drops the internal resolution under load to hold the frame rate.");

    section("Image");
    if (g->has_antialiasing)  // psxrecomp: 0/1 linear (smoothed) present scaling
        row_toggle("Smooth scaling", &io->antialiasing, "Linear filtering when the image is scaled to the window.");
    if (g->has_texture_filter) {
        static const char* kTf[] = {"Nearest (original)", "Bilinear"};
        row_combo("Texture filtering", &io->texture_filter, kTf, 2);
    }
    if (g->has_fmv_filter) {
        static const char* kFmv[] = {"Nearest", "Bilinear", "Sharp", "Bicubic"};
        int v = io->fmv_filter >= 1 && io->fmv_filter <= 4 ? io->fmv_filter - 1 : 1;
        if (row_combo("Movie filter", &v, kFmv, 4)) io->fmv_filter = v + 1;
    }
    if (g->has_scanlines) {
        row_toggle("CRT scanlines", &io->scanlines);
        if (io->scanlines) row_slider("Scanline strength", &io->scanline_strength_pct, 0, 100, "%d%%");
    }
    if (g->has_geometry_precision) {
        row_toggle("Geometry correction", &io->geometry_correction, "Sub-pixel vertex precision (PGXP).");
        row_toggle("Perspective-correct textures", &io->perspective_texturing);
    }

    if (g->has_render_pipeline) {
        section("Pipeline");
        row_toggle("Render thread", &io->render_thread);
        row_toggle("Present thread", &io->present_thread);
        restart_chip(offsetof(RecompLauncherCSettings, render_thread), 2 * sizeof(int));
        row_toggle("Frame generation", &io->frame_generation, "Reprojected in-between frames for high refresh displays.");
    }

    const TitleLayer& t = *title;
    if (t.graphics_feature_count && mods.available()) {
        section("R4 enhancements");
        for (int i = 0; i < t.graphics_feature_count; ++i) {
            ModFeature* f = mods.find(t.graphics_features[i].package_id, t.graphics_features[i].feature_id);
            if (!f) continue;
            ImGui::PushID(i);
            int on = f->info.enabled;
            if (row_toggle(f->info.name, &on, f->info.description[0] ? f->info.description : nullptr))
                mods.set_enabled(*f, on != 0);
            for (auto& o : f->options) mod_option_row(mods, *f, o);
            ImGui::PopID();
        }
    }
    draw_skin_picker(*this);
}

void draw_skin_picker(App& app) {
    const std::vector<std::string> skins = list_skins(app.s.assets_dir);
    if (skins.empty()) return;
    section("Menu skin");
    int cur = 0;
    std::vector<const char*> c;
    for (size_t i = 0; i < skins.size(); ++i) {
        c.push_back(skins[i].c_str());
        if (app.skin_dir.size() >= skins[i].size() &&
            app.skin_dir.compare(app.skin_dir.size() - skins[i].size(), skins[i].size(), skins[i]) == 0)
            cur = static_cast<int>(i);
    }
    if (row_combo("Skin", &cur, c.data(), static_cast<int>(c.size()), "Edits to skin.json reload live."))
        load_skin_named(app, skins[cur]);
}

// ---------------------------------------------------------------- Mods

void App::draw_mods() {
    title_text("Mods", mode == Mode::Overlay
                           ? "Options a mod can change on a running game apply now; the rest are saved for the next start."
                           : "Features from the bundled and installed mod packages. Changes apply when you press Play.");
    if (!mods.available()) {
        ImGui::TextDisabled("This build has no mod support.");
        return;
    }
    const float w = ImGui::GetContentRegionAvail().x;
    ImGui::BeginChild("##modlist", ImVec2(w * 0.5f, 0), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened);
    for (const ModGroup& grp : mods.groups) {
        section(grp.name.c_str());
        for (int idx : grp.features) {
            ModFeature& f = mods.features[idx];
            ImGui::PushID(idx);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float rw = ImGui::GetContentRegionAvail().x;
            if (ImGui::Selectable("##f", selected_feature == idx, 0, ImVec2(rw - 70, theme().row_h)))
                selected_feature = idx;
            if (ImGui::IsItemFocused()) selected_feature = idx;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float fs = ImGui::GetFontSize();
            dl->PushClipRect(p, ImVec2(p.x + rw - 96, p.y + theme().row_h), true);
            dl->AddText(theme().body, fs, ImVec2(p.x + 8, p.y + theme().row_h * 0.1f),
                        ImGui::ColorConvertFloat4ToU32(theme().text), f.info.name);
            dl->AddText(theme().body, fs * 0.75f, ImVec2(p.x + 8, p.y + theme().row_h * 0.58f),
                        ImGui::ColorConvertFloat4ToU32(theme().text_dim), f.info.package_name);
            dl->PopClipRect();
            if (f.info.has_error || !f.diagnostics.empty())
                dl->AddCircleFilled(ImVec2(p.x + rw - 90, p.y + theme().row_h * 0.5f), 4,
                                    ImGui::ColorConvertFloat4ToU32(theme().warn));
            ImGui::SameLine(rw - 62);
            int on = f.info.enabled;
            const ImVec2 tp = ImGui::GetCursorScreenPos();
            const float h = 26;
            ImGui::SetCursorScreenPos(ImVec2(tp.x, p.y + (theme().row_h - h) * 0.5f));
            if (ImGui::InvisibleButton("##tog", ImVec2(50, h))) {
                on = !on;
                mods.set_enabled(f, on != 0);
            }
            const ImVec2 a = ImGui::GetItemRectMin();
            dl->AddRectFilled(a, ImVec2(a.x + 50, a.y + h),
                              ImGui::ColorConvertFloat4ToU32(on ? theme().accent : theme().surface_hi), h * 0.5f);
            dl->AddCircleFilled(ImVec2(on ? a.x + 50 - h * 0.5f : a.x + h * 0.5f, a.y + h * 0.5f), h * 0.5f - 3,
                                ImGui::ColorConvertFloat4ToU32(theme().text));
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##moddetail", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    if (selected_feature < 0 && !mods.features.empty()) selected_feature = 0;
    if (selected_feature >= 0 && selected_feature < static_cast<int>(mods.features.size())) {
        ModFeature& f = mods.features[selected_feature];
        ImGui::PushFont(theme().bold, theme().body_size * 1.35f);
        ImGui::TextWrapped("%s", f.info.name);
        ImGui::PopFont();
        ImGui::TextDisabled("%s %s%s%s", f.info.package_name, f.info.package_version,
                            f.info.author[0] ? " · " : "", f.info.author);
        if (f.info.channel == RECOMP_MOD_CHANNEL_EXPERIMENTAL) chip("Experimental", theme().warn);
        if (f.info.description[0]) ImGui::TextWrapped("%s", f.info.description);
        if (f.info.status[0]) ImGui::TextColored(theme().text_dim, "%s", f.info.status);
        if (mods.needs_restart.count(std::string(f.info.package_id) + "/" + f.info.id))
            chip("Applies after restart", theme().warn);
        if (!f.options.empty()) {
            section("Options");
            for (auto& o : f.options) {
                ImGui::PushID(o.info.id);
                mod_option_row(mods, f, o);
                ImGui::PopID();
            }
        }
        if (!f.diagnostics.empty()) {
            section("Problems");
            for (auto& d : f.diagnostics)
                ImGui::TextColored(d.severity == RECOMP_MOD_DIAGNOSTIC_ERROR ? theme().bad : theme().warn, "%s",
                                   d.message);
        }
        if (f.info.source_url[0]) {
            section("Source");
            ImGui::TextDisabled("%s", f.info.source_url);
        }
    }
    ImGui::EndChild();
}

// ---------------------------------------------------------------- Controls

void App::draw_controls() {
    RecompLauncherCSettings* io = s.io;
    const RecompLauncherCGameInfo* g = s.game;
    const TitleLayer& t = *title;
    title_text("Controls", nullptr);
    if (!io || !g) return;

    // Modern / Classic (title layer -> mod option)
    if (ModFeature* f = mods.find(t.modern_controls.package_id, t.modern_controls.feature_id)) {
        for (auto& o : f->options) {
            if (!t.modern_option || std::strcmp(o.info.id, t.modern_option)) continue;
            section("Control scheme");
            const bool modern = f->info.enabled && !std::strcmp(o.info.value, t.modern_value);
            const float bw = (ImGui::GetContentRegionAvail().x - 10) * 0.5f;
            if (big_button("Modern", ImVec2(bw, 64), modern)) {
                if (!f->info.enabled) mods.set_enabled(*f, true);
                mods.set_option(*f, o, t.modern_value);
            }
            ImGui::SameLine(0, 10);
            if (big_button("Classic", ImVec2(bw, 64), !modern)) mods.set_option(*f, o, t.classic_value);
            ImGui::TextDisabled(modern ? "Analog steering and triggers, camera on the right stick."
                                       : "The original 1998 button layout.");
        }
    }

    // Player tabs
    section("Players");
    const int players = std::min(t.max_players, g->num_players > 0 ? g->num_players : t.max_players);
    for (int p = 0; p < players; ++p) {
        if (p) ImGui::SameLine(0, 8);
        char lbl[8];
        std::snprintf(lbl, sizeof(lbl), "P%d", p + 1);
        if (big_button(lbl, ImVec2(72, 40), controls_player == p)) controls_player = p;
    }
    const int pl = controls_player;
    {
        static const char* kSrc[] = {"None", "Keyboard", "Gamepad"};
        row_combo("Input device", &io->player_src[pl], kSrc, 3);
        if (io->player_src[pl] == 2) {
            int count = 0;
            SDL_JoystickID* ids = SDL_GetGamepads(&count);
            std::vector<std::string> names{"Any connected gamepad"};
            std::vector<std::string> guids{""};
            for (int i = 0; ids && i < count; ++i) {
                const char* n = SDL_GetGamepadNameForID(ids[i]);
                char gs[40];
                SDL_GUIDToString(SDL_GetGamepadGUIDForID(ids[i]), gs, sizeof(gs));
                names.push_back(n ? n : "Gamepad");
                guids.push_back(gs);
            }
            SDL_free(ids);
            int cur = 0;
            for (size_t i = 0; i < guids.size(); ++i)
                if (guids[i] == io->player_gamepad_guid[pl]) cur = static_cast<int>(i);
            std::vector<const char*> c;
            for (auto& n : names) c.push_back(n.c_str());
            if (row_combo("Gamepad", &cur, c.data(), static_cast<int>(c.size())))
                std::snprintf(io->player_gamepad_guid[pl], sizeof(io->player_gamepad_guid[pl]), "%s",
                              guids[cur].c_str());
        }
        // Controller profile (DualShock / digital / NeGcon / JogCon)
        std::vector<const char*> labels;
        int cur = -1;
        for (int i = 0; i < t.controller_count; ++i) {
            labels.push_back(t.controllers[i].label);
            const ControllerProfile& cp = t.controllers[i];
            ModFeature* f = cp.feature.package_id ? mods.find(cp.feature.package_id, cp.feature.feature_id) : nullptr;
            if (cp.pad_mode == io->pad_mode[pl] && (!f || f->info.enabled) && cur < 0) cur = i;
        }
        if (cur < 0) cur = 0;
        if (g->pad_mode_supported &&
            row_combo("Controller", &cur, labels.data(), static_cast<int>(labels.size()), t.controllers[cur].detail)) {
            const ControllerProfile& cp = t.controllers[cur];
            io->pad_mode[pl] = cp.pad_mode;
            if (cp.feature.package_id)
                if (ModFeature* f = mods.find(cp.feature.package_id, cp.feature.feature_id)) mods.set_enabled(*f, true);
        }
        if (g->has_deadzone_pct) row_slider("Stick deadzone", &io->deadzone[pl], 0, 50, "%d%%");
    }
    if (players > 2) {
        section("Multitap");
        row_toggle("Multitap (3-4 players)", &io->multitap_enabled);
        row_toggle("Analog on multitap", &io->multitap_analog);
    }

    // Bindings table
    section("Bindings");
    if (ImGui::BeginTable("##binds", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("PlayStation", 0, 1.2f);
        ImGui::TableSetupColumn("Key", 0, 1.0f);
        ImGui::TableSetupColumn("Alt key", 0, 1.0f);
        ImGui::TableSetupColumn("Gamepad", 0, 1.0f);
        ImGui::TableHeadersRow();
        const int kp = pl < kPsxKeyboardPlayers ? pl : 0;
        PadMapping* pm = s.pads.for_guid("", false);
        for (int i = 0; i < kPsxInputCount; ++i) {
            ImGui::PushID(i);
            ImGui::TableNextRow(0, 36);
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(kPsxInputs[i].label);
            auto cell = [&](const char* id, const std::string& v, Capture::Kind k, bool alt) {
                ImGui::TableNextColumn();
                if (ImGui::Button((std::string(v.empty() ? "-" : v) + "##" + id).c_str(), ImVec2(-1, 0))) {
                    capture.kind = k;
                    capture.player = kp;
                    capture.input = i;
                    capture.alt = alt;
                    capture.started = time;
                }
            };
            cell("k", s.keys.player[kp][i].primary, Capture::Key, false);
            cell("a", s.keys.player[kp][i].alt, Capture::Key, true);
            cell("p", pm->source[i], Capture::PadSource, false);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (ImGui::Button("Reset bindings to defaults")) {
        s.keys = KeyboardBinds::defaults();
        s.pads.global = PadMapping::defaults();
        s.binds_dirty = true;
    }

    // Shortcuts (assist bindings: Rewind, Save states, Fast-forward...)
    if (g->assist_binding_count > 0 && g->assist_binding_labels) {
        section("Shortcuts");
        if (g->has_rewind_depth) {
            row_toggle("Rewind", &io->rewind_enabled);
            if (io->rewind_enabled) row_slider("Rewind depth", &io->rewind_depth, 10, 600, "%d snapshots");
        }
        for (int a = 0; a < g->assist_binding_count && a < RECOMP_LAUNCHER_MAX_ASSIST_BINDINGS; ++a) {
            ImGui::PushID(1000 + a);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(g->assist_binding_labels[a]);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.52f);
            const std::string v = describe_pad_value(io->assist_pad_bind[a]);
            if (ImGui::Button((v + "##pv").c_str(), ImVec2(240, 0))) {
                capture.kind = Capture::PadValue;
                capture.input = a;
                capture.combo = 0;
                capture.started = time;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear")) io->assist_pad_bind[a] = 0;
            ImGui::PopID();
        }
    }
}

}  // namespace r4l
