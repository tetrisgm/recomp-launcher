# recomp-ui parity

Every recomp-ui capability that a PSX host can reach, where recomp-ui does it
(`LI` = `src/common/backends/imgui/launcher_imgui.cpp`, `LM` = `launcher_model.c`),
and how recomp-launcher does it. The surface key in the last column is what a title
uses to show or hide the capability (docs/SUPPORTED.md).

Status: **Done** = implemented and reachable when the title shows the key. Audit
of recomp-ui `3fa96c3` (contains the R4 pin `7e884a2`).

Verified column (2026-10-08): **verified on R4** = the flow was driven at least
once against the real R4 runtime (hidden window, `R4L_SCRIPT`, netplay against a
local recomp-net lobby server, LAN race launched with lockstep armed on both peers).
Cross-machine (2026-10-09, Mac + Windows PC, R4 built with this launcher on both):
direct IP both directions, lobby-server hosted both directions, and a 200 s race from
the race savestate; rollback digests matched on every compared tick, 0 dispatch misses.
**stand-in only** = exercised on `r4l-fake-host` only; most of these are hidden or
not wired on R4 (no BIOS choice, precompiled, single disc, no Discord/automatch).
47 rows verified on R4, 27 stand-in only.

`tests/parity_check.py` (ctest `r4l_parity`) checks that every GameInfo,
NetplayCallbacks and ModProvider member is used by the launcher, or listed in the
last section.

| Area | Capability | recomp-ui | recomp-launcher | Key | Verified |
|---|---|---|---|---|---|
| Home | Game, controller and save panels | LI draw_dashboard | Home: disc card, preset/controls/mods tiles; save → Settings › Memory cards | — | verified on R4 |
| Home | Box art, verdict, disc selector | LI draw_game_panel, draw_verdict_block | Home disc card (box art from `boxart_path`), verdict chips; Disc setup › Disc selection | `disc.multi` | verified on R4 |
| Home | PLAY / RESUME (`in_session`) | LI draw_footer | Play button says RESUME when `in_session` | — | verified on R4 |
| Home | Skip launcher on boot | LI draw_footer | Settings › Skip this launcher next time | `system.skip_launcher` | verified on R4 |
| Home | Restore defaults (`default_settings`) | LI draw_restore_defaults_modal | Settings › Restore defaults (host defaults, else as opened) | — | verified on R4 |
| Setup | First-run wizard (`needs_setup`, `setup_wizard_supported`, `persist_setup[_discs]`) | LI draw_setup_wizard_modal | Disc setup page opens automatically; Continue persists | `disc.setup` | verified on R4 |
| Setup | Find the disc without asking | — (new) | Auto-scan of game folder, Games, Downloads, ROM folders, EmuDeck, SD cards; first image `disc_verify` accepts with the title's serial | `disc.autoscan` | verified on R4 |
| Setup | Prepare / generate with progress | LI 13007, LM poll_prepare_disc | Disc setup › Generate and build (worker thread, progress, host strings) | `disc.prepare` | stand-in only |
| Setup | Rebuild, relaunch | LI draw_setup_progress_modal | Same job; RELAUNCH + `recomp_launcher_relaunch_exe` | `disc.prepare` | stand-in only |
| Setup | Toolchain download / repair / update | LI 13007 | Disc setup › Download build tools; About › Updates (update check, repair note) | `disc.toolchain`, `about.updates` | stand-in only |
| Setup | PGO "Optimize movie playback" | LI draw_pgo_confirm_modal | Settings › Movies | `disc.pgo` | stand-in only |
| Setup | FMV timing | LI draw_fmv_timing_confirm_modal | Settings › Movies | `disc.fmv_timing` | stand-in only |
| Setup | "Not runnable" reason | LI generate_disabled_reason | Line above Continue says why it is off | `disc.setup` | verified on R4 |
| Disc | Verify verdict, serial, region, tracks, netplay detail | LI draw_verdict_block | Disc setup + Home chips | `disc.setup` | verified on R4 |
| Disc | Multi-disc selection (`discs`, `disc_index`) | LI draw_disc_selector | Disc setup › Disc selection | `disc.multi` | stand-in only |
| Disc | SBI import | LI 2069–2520 | Disc setup › Import .sbi | `disc.sbi` | stand-in only |
| Disc | Picker patterns (`rom_patterns`, `rom_filter_desc`, `rom_noun`) | LI request_rom_picker | Native dialog with the host's patterns | — | stand-in only |
| BIOS | Select, Use OpenBIOS, verify (`bios_verify`) | LI draw_system_controls | Settings › BIOS, Disc setup › BIOS | `bios.select` | stand-in only |
| BIOS | Prepare BIOS (`bios_prepare_*`) | LI draw_bios_confirm_modal | Settings › BIOS › Prepare | `bios.prepare` | stand-in only |
| Memory cards | 15-block view (`memcard_inspect`), enable, choose, new/format | LI panel_save_draw, memcard_format.c | Settings › Memory cards (own formatter, `core/memcard.cpp`) | `system.memcards` | verified on R4 |
| Video | Graphics preset, Custom, Re-detect (`quality_*`) | LI draw_quality_preset_row | Graphics › Preset, plus **Auto** (detected) | `graphics.preset`, `graphics.redetect` | verified on R4 (psxrecomp integrate/r4-ultra-defaults scratch build + sample [quality.*] tables) |
| Video | Window size | LI row_window_scale | Graphics › Window width | `graphics.window_size` | verified on R4 |
| Video | Renderer (`renderer_labels/ids`) | LI 4004 | Graphics › Renderer (restart-only in-game) | `graphics.renderer` | verified on R4 |
| Video | Render / present thread, Smooth motion | LI 3853 | Graphics › Pipeline, Smooth motion | `graphics.render_thread`, `graphics.present_thread`, `graphics.frame_generation` | verified on R4 |
| Video | Internal resolution / supersampling | LI 4040 / 4072 | Graphics › Resolution | `graphics.internal_resolution`, `graphics.supersampling` | verified on R4 (Auto-pinned; supersampling not offered by R4) |
| Video | Dynamic resolution + lowest resolution | LI 4086 | Graphics › Resolution | `graphics.dynamic_resolution` | verified on R4 (Auto-pinned) |
| Video | Screen layout (windowed / fullscreen / exclusive) | LI 4112 | Graphics › Screen | `graphics.fullscreen` | verified on R4 |
| Video | Texture / FMV filtering, antialiasing | LI 4134–4169 | Graphics › Image | `graphics.texture_filter`, `graphics.fmv_filter`, `graphics.antialiasing` | verified on R4 |
| Video | Perspective textures / geometry | LI 4196 | Graphics › Image | `graphics.geometry` | stand-in only |
| Video | Screen model, scanlines + strength | LI 4221–4243 | Graphics › Image | `graphics.screen_kind`, `graphics.scanlines` | verified on R4 |
| Video | V-Sync | LI 4269 | Graphics › V-Sync | `graphics.vsync` | verified on R4 |
| Video | Skip FMVs | LI 4295 | Settings › Skip movies | `system.skip_fmv` | stand-in only |
| Video | Rewind on/off, buffer, interval | LI 4301 | Settings › Rewind | `system.rewind` | verified on R4 |
| Video | Turbo loads (no row in recomp-ui) | — | Settings › Fast loading | `system.turbo_loads` | stand-in only |
| Video | View / aspect (`aspect_*`, `widescreen_supported`, `adaptive_view_supported`) | LI draw_aspect_row | Graphics › Aspect ratio (when the host offers it) | `graphics.aspect` | stand-in only |
| Video | Frame interpolation / blend, run-ahead, shader, integer scale | LI (other consoles) | Graphics rows behind their `has_*` flags | `graphics.frame_interp`, `graphics.frame_blend`, `graphics.run_ahead`, `graphics.shader`, `graphics.integer_scale` | stand-in only |
| Audio | Sample rate, volume, high-quality SPU, output device | LI 4411–4441, 4425 | Settings › Audio | `audio.frequency`, `audio.volume`, `audio.spu_hq`, `audio.device` | verified on R4 |
| Audio | Language (`language_labels`) | LI 4489 | Settings › Language | `system.language` | stand-in only |
| Input | Player source, gamepad pick (GUID / instance) | LI draw_player_panel | Controls › Input device, Gamepad | `controls.devices` | stand-in only |
| Input | Pad mode with locks (`pad_mode_*`, `locked_pad_mode`, `lock_device`) | LI pad_mode_selector | Controls › Controller (title profiles: DualShock, Digital, NeGcon, JogCon) | `controls.profile` | verified on R4 |
| Input | Deadzone, multitap (+ analog) | LI 5658, 4517 | Controls | `controls.deadzone`, `controls.multitap` | verified on R4 |
| Input | Rebind keyboard + gamepad, two binds each, Map All, Reset | LI draw_controller_config_view | Controls › Bindings (key, alt key, gamepad, alt gamepad; Map all keys / buttons) | `controls.bindings` | verified on R4 |
| Input | Host shortcut buttons (assist pad binds, defaults, direct action) | LI draw_controller_assist_shortcuts | Controls › Shortcut buttons (combo capture, Default, keyboard default tooltip) | `controls.assist` | verified on R4 |
| Hotkeys | config.ini `[KeyMap]` (incl. OpenLauncher) | LI draw_hotkeys_controls | Settings › Shortcut keys (capture with modifiers) | `controls.hotkeys` | verified on R4 |
| Netplay | Lobby server, addresses | LI draw_netplay_network_modal | Netplay › Lobby server | `netplay.lobby_server` | verified on R4 |
| Netplay | Connect, list, scope (All / LAN / Online) | LI np_connect_and_list | Netplay › Open lobbies | `netplay.browse` | verified on R4 |
| Netplay | Host (password, LAN, seats, `create_max_slots`, `create_default_rollback`) | LI draw_netplay_host_modal | Netplay › Host a race | `netplay.host` | verified on R4 |
| Netplay | Join by code, password, by address, resume room / endpoint | LI np_join_selected, password, direct modals | Netplay › Join a race, password prompt | `netplay.join` | verified on R4 |
| Netplay | Seats, kick, move, swap | LI draw_lobby_seat_row | Lobby seat cards | `netplay.lobby`, `netplay.seat_swap` | verified on R4 |
| Netplay | Ready, start, launch, launch gate | LI np_lobby_start, np_try_launch | Lobby | `netplay.lobby` | verified on R4 |
| Netplay | Lobby chat, server chat, players online, country | LI draw_lobby_chat, draw_server_chat, draw_netplay_online_panel | Lobby chat; Players online + server chat (country code shown as text) | `netplay.chat`, `netplay.online` | verified on R4 |
| Netplay | Report, block (`chat_report`, `set_blocks`, moderation.ini) | LI np_player_menu | Right-click a player: Report… / Block; blocked chat hidden | `netplay.moderation` | stand-in only |
| Netplay | Account (Discord) | LI draw_account_section | Netplay › Account | `netplay.account` | stand-in only |
| Netplay | Automatch (rulesets, queue, accept) | LI draw_netplay_automatch_modal | Netplay › Quick match | `netplay.automatch` | stand-in only (needs a signed-in account; no R4 ruleset) |
| Netplay | Match settings (delay, prediction, rollback, relay, TURN, multitap analog, relay status) | LI draw_lobby_match_settings | Lobby › Match settings | `netplay.tuning` | verified on R4 (relay used for the race) |
| Netplay | Spectators | LI 7859, 8505 | Lobby › Spectators | `netplay.spectators` | verified on R4 (hidden in R4 surface; psxrecomp #607 fixes start; observer stalls on relay wire holes) |
| Netplay | Memory card offer / guest card | LI draw_lobby_memcard_toggle | Lobby › Memory cards | `netplay.memcard` | verified on R4 |
| Netplay | Lobby mods, downloads, transfers | LI draw_lobby_mods_popup | Lobby › Lobby mods | `netplay.mod_transfer` | stand-in only (R4 netplay is vanilla) |
| Netplay | Session variant, Link lobby kind | (header; R4 host does not wire) | Lobby › Mode, Link battle toggle | `netplay.variant` | stand-in only |
| Netplay | Handoff ingest (`RECOMP_NETPLAY_LAUNCH`) | launcher_ng_capi.c | Same, before any window | — | stand-in only |
| Mods | Packages, versions, install, remove, legacy switch / options, author links | LI draw_mod_packages | Mods › Packages | `mods.install`, `mods.versions` | verified on R4 |
| Mods | Features: search, enable, options, Disable all | LI draw_mod_features | Mods (search, Turn all off) | `mods.list` | verified on R4 |
| Mods | Resource folders | LI pick_mod_feature_resource | Mods › Files | `mods.resources` | verified on R4 |
| Mods | Diagnostics, catalog diagnostics | LI draw_mod_feature_diagnostics | Mods › Problems, catalog lines | `mods.diagnostics` | stand-in only |
| Mods | Channels, hidden filtering | LI mod_channel_tag, launcher_mod_visibility.c | Experimental / Developer chips; `hide_hidden_features`; developer channel only in developer builds | `mods.experimental` | stand-in only |
| Mods | Commit on Play, prepared commit (`try_commit`, `preparation_revision`) | LI mod_commit_launch | Commit on Play, fast path when the prepared plan is current | — | verified on R4 |
| Window | Launcher size persistence, icon | launcher_window_size.h | launcher-window.ini, `window_icon_path` | — | stand-in only |
| Debug | Scripted driving / screenshots (`LNG_SCRIPT`) | launcher_debug.c | `R4L_SCREEN`, `R4L_SCREENSHOT`, `r4l-skin-render`, `R4L_SURFACE=all` | — | verified on R4 |
| Errors | Setup errors, netplay banners, mod errors | LI 2336, 12852, 8235 | Footer status line, netplay status, modals | — | verified on R4 |
| i18n | `ui_text` tables (compile-time language) | launcher_i18n.cpp | `tr()` + `assets/i18n/<lang>.json` (Italian included), `-DR4L_UI_LANGUAGE` or `R4L_LANGUAGE` | — | verified on R4 |
| Emoji / flags | Flag sprites, emoji glyph fallback | common/emoji | Country codes as text; Lato + ImGui default glyphs | — | stand-in only |
| Theme | PlayStation theme (`theme`, `platform`) | launcher_theme.h | Skins (default, R4); `theme` names a fallback skin; `platform` on Home | `graphics.skin` | verified on R4 |
| About | Version, logs | (none) | About › Version, Logs | `about.version`, `about.logs` | verified on R4 |
| In-game | (none) | — | In-game overlay on psxrecomp #598–#603 | — | verified on R4 |

## Not wired by psxrecomp / not applicable

These ABI members exist for other consoles' hosts (cartridges, SNES, N64, NES,
DS) or for recomp-ui pages the PSX host never enables. No PSX title sets them,
so the launcher has no PSX behaviour to reproduce; they are listed so the parity
check stays honest.

- Cartridge identity and patching: `expected_crc`, `has_expected_crc`,
  `known_sha256`, `num_known_sha256`, `known_sha1_hex`, `num_known_sha1`,
  `rom_patch_supported`, `rom_patch_note`, `rom_patch_cache_dir`,
  `rom_patch_required_sha1`, `sram_path`.
- SNES MSU-1: `msu1_supported`, `msu1_note`, `msu1_patch_path`.
- Password saves (NES): `password_save_path`, `password_save_label`,
  `password_sram_path`, `password_sram_label`, `password_sram_size`,
  `password_sram_offset`.
- N64 Transfer Pak: `tpak_slots`, `tpak_inspect`.
- Light gun / sensors / stylus: `zapper`, `has_gyro_controls`,
  `has_solar_sensor`, `has_virtual_stylus`.
- Other consoles' video rows: `has_sharp_filter`, `has_affine_filter`,
  `has_snes_display_aspect`, `display_layout_labels`, `num_display_layouts`,
  `hdpack_supported`, `netplay_view_labels`, `num_netplay_view_labels`.
- Assist Tools page (recomp-ui hides it for PSX; the PSX shortcuts are in
  Controls): `has_assist_tools`, `assist_tools_note`, `assist_fast_forward_min`,
  `assist_fast_forward_max`.
- Settings-struct bindings (`settings_bindings`): PSX keeps bindings in
  keybinds.ini / input.ini, which the launcher edits directly.
