// screen_netplay.cpp — host / join (code or address) / lobby with seats P1..Pn.
#include "ui.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace r4l {

void App::draw_netplay() {
    screen_title("Netplay");
    const RecompLauncherCGameInfo* g = s.game;
    const RecompLauncherCNetplayCallbacks* np = g ? g->netplay : nullptr;
    if (!g || !g->netplay_supported || !np) {
        ImGui::TextDisabled("Netplay is not available in this build.");
        return;
    }
    void* c = np->ctx;
    RecompLauncherCSettings* io = s.io;
    auto call_err = [&](const char* what) {
        const char* e = np->last_error ? np->last_error(c) : nullptr;
        np_status = std::string(what) + (e && *e ? ": " + std::string(e) : "");
        if (np->clear_last_error) np->clear_last_error(c);
    };

    // Identity + connection
    section("You");
    ImGui::SetNextItemWidth(320);
    if (ImGui::InputTextWithHint("Player name", "Racer", io->netplay_player_name, sizeof(io->netplay_player_name)) &&
        np->set_player_name)
        np->set_player_name(c, io->netplay_player_name);
    const bool connected = np->connected && np->connected(c);
    const bool connecting = np->connecting && np->connecting(c);
    ImGui::SameLine(0, 24);
    chip(connected ? "Online" : connecting ? "Connecting..." : "Offline",
         connected ? theme().ok : connecting ? theme().warn : theme().text_dim);
    if (!connected && !connecting) {
        ImGui::SameLine();
        if (ImGui::Button("Connect") && np->connect) {
            if (np->set_player_name) np->set_player_name(c, io->netplay_player_name);
            if (!np->connect(c)) call_err("Connect failed");
        }
    }
    if (!np_status.empty()) ImGui::TextColored(theme().warn, "%s", np_status.c_str());

    const bool in_lobby = np->in_lobby && np->in_lobby(c);
    if (in_lobby) {
        // ---------------- Lobby
        const bool host = np->is_host && np->is_host(c);
        const int seats = np->lobby_max_slots ? std::max(2, np->lobby_max_slots(c)) : title->netplay_seats;
        section(host ? "Your lobby" : "Lobby");
        RecompLauncherCNetplayMember m[RECOMP_LAUNCHER_NETPLAY_MAX_MEMBERS + 1] = {};
        bool filled[RECOMP_LAUNCHER_NETPLAY_MAX_MEMBERS + 1] = {};
        const int n = np->member_count ? np->member_count(c) : 0;
        for (int i = 0; i < n; ++i) {
            RecompLauncherCNetplayMember x{};
            if (np->member_get && np->member_get(c, i, &x) && x.slot >= 0 &&
                x.slot <= RECOMP_LAUNCHER_NETPLAY_MAX_MEMBERS && !x.is_spectator) {
                m[x.slot] = x;
                filled[x.slot] = true;
            }
        }
        const float cw = (ImGui::GetContentRegionAvail().x - (seats - 1) * 10) / seats;
        for (int sl = 0; sl < seats && sl <= RECOMP_LAUNCHER_NETPLAY_MAX_MEMBERS; ++sl) {
            if (sl) ImGui::SameLine(0, 10);
            ImGui::PushID(sl);
            ImGui::BeginChild("##seat", ImVec2(cw, 0), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);
            ImGui::PushFont(theme().bold, theme().body_size * 1.3f);
            ImGui::TextColored(theme().accent, "P%d", sl + 1);
            ImGui::PopFont();
            if (filled[sl]) {
                ImGui::TextUnformatted(m[sl].display_name);
                if (m[sl].is_host) chip("Host", theme().accent2);
                else chip(m[sl].ready ? "Ready" : "Not ready", m[sl].ready ? theme().ok : theme().text_dim);
                if (m[sl].latency_ms > 0) ImGui::TextDisabled("%d ms", m[sl].latency_ms);
                if (host && !m[sl].is_local && np->kick_member && ImGui::SmallButton("Kick")) np->kick_member(c, sl);
                if (!m[sl].is_local && np->seat_swap_request) {
                    if (!host) ImGui::SameLine();
                    if (ImGui::SmallButton("Swap seats") && !np->seat_swap_request(c, sl))
                        call_err("Swap not possible");
                }
            } else {
                ImGui::TextDisabled("Open seat");
                if (np->seat_move_self && ImGui::SmallButton("Sit here")) np->seat_move_self(c, sl);
            }
            ImGui::EndChild();
            ImGui::PopID();
        }
        // Seat swaps: answer an incoming request, or show ours as pending.
        if (np->seat_swap_incoming) {
            char who[64] = {0};
            int from = -1;
            if (np->seat_swap_incoming(c, who, sizeof(who), &from)) {
                ImGui::TextColored(theme().accent, "%s wants to swap with you (P%d)", who[0] ? who : "A player", from + 1);
                ImGui::SameLine();
                if (ImGui::SmallButton("Accept") && np->seat_swap_respond) np->seat_swap_respond(c, 1);
                ImGui::SameLine();
                if (ImGui::SmallButton("Decline") && np->seat_swap_respond) np->seat_swap_respond(c, 0);
            } else if (np->seat_swap_outgoing && np->seat_swap_outgoing(c)) {
                ImGui::TextDisabled("Swap requested, waiting for an answer...");
                ImGui::SameLine();
                if (ImGui::SmallButton("Cancel") && np->seat_swap_clear) np->seat_swap_clear(c);
            }
        }
        ImGui::Dummy(ImVec2(0, 8));
        if (!host && np->set_ready) {
            const bool r = np->local_ready && np->local_ready(c);
            if (big_button(r ? "Ready!" : "Ready up", ImVec2(200, 52), r)) np->set_ready(c, r ? 0 : 1);
            ImGui::SameLine();
        }
        if (host && np->request_start) {
            const bool all = np->all_ready && np->all_ready(c);
            ImGui::BeginDisabled(!all || !s.media_ready());
            if (big_button("Start race", ImVec2(200, 52), true) && !np->request_start(c, io)) call_err("Start failed");
            ImGui::EndDisabled();
            ImGui::SameLine();
        }
        if (ImGui::Button("Leave", ImVec2(120, 52)) && np->leave) np->leave(c);
        if (host && np->input_delay_get && np->input_delay_set) {
            int d = np->input_delay_get(c);
            if (row_slider("Input delay", &d, 0, 8, "%d frames")) np->input_delay_set(c, d);
        }
        if (host && np->rollback_get && np->rollback_set) {
            int r = np->rollback_get(c);
            if (row_toggle("Rollback", &r)) np->rollback_set(c, r);
        }
        // Chat
        if (np->chat_count && np->chat_get) {
            section("Chat");
            ImGui::BeginChild("##chat", ImVec2(0, 140), ImGuiChildFlags_AlwaysUseWindowPadding);
            const int cc = np->chat_count(c);
            for (int i = std::max(0, cc - 50); i < cc; ++i) {
                RecompLauncherCNetplayChatMessage msg{};
                if (!np->chat_get(c, i, &msg)) continue;
                if (msg.is_system) ImGui::TextDisabled("%s", msg.text);
                else ImGui::TextWrapped("%s: %s", msg.from, msg.text);
            }
            ImGui::EndChild();
            ImGui::SetNextItemWidth(-90);
            if (ImGui::InputText("##say", np_chat, sizeof(np_chat), ImGuiInputTextFlags_EnterReturnsTrue) && np_chat[0]) {
                if (np->chat_send) np->chat_send(c, np_chat);
                np_chat[0] = 0;
            }
        }
        // Everyone launches together once the host's start goes through.
        if (np->launch_pending && np->launch_pending(c) && np->fill_launch) {
            RecompLauncherCNetplayLaunch l{};
            if (np->fill_launch(c, &l) && l.enabled) {
                std::string err;
                if (g->mods && g->mods->commit_netplay &&
                    !g->mods->commit_netplay(g->mods->ctx, s.primary_disc().c_str())) {
                    np_status = "Mods could not be prepared for this match.";
                } else {
                    io->netplay_launch = l;
                    s.persist_media(&err);
                    s.commit_files(&err);
                    s.outcome = Outcome::Launch;
                }
            }
            if (np->clear_launch_pending) np->clear_launch_pending(c);
        }
        return;
    }

    // ---------------- Host / Join
    const float half = (ImGui::GetContentRegionAvail().x - 16) * 0.5f;
    ImGui::BeginChild("##host", ImVec2(half, 330), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PushFont(theme().bold, theme().body_size * 1.2f);
    ImGui::TextUnformatted("Host a race");
    ImGui::PopFont();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##ln", "Lobby name", np_lobby_name, sizeof(np_lobby_name));
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##pw", "Password (optional)", np_password, sizeof(np_password),
                             ImGuiInputTextFlags_Password);
    static const char* kSeats[] = {"2 players", "3 players", "4 players"};
    int si = np_seats - 2;
    if (row_combo("Seats", &si, kSeats, std::min(3, title->netplay_seats - 1))) np_seats = si + 2;
    int lan = np_lan_only ? 1 : 0;
    if (row_toggle("LAN only", &lan)) np_lan_only = lan != 0;
    ImGui::BeginDisabled(!s.media_ready());
    if (big_button("Host", ImVec2(-1, 48), true) && np->create) {
        char endpoint[96] = {0};
        if (np->set_player_name) np->set_player_name(c, io->netplay_player_name);
        const int rc = np->create(c, np_lobby_name, endpoint, np_password, io, np_lan_only ? 1 : 0, np_seats);
        if (rc <= 0) call_err(rc == -4 ? "Port is busy" : "Could not host");
    }
    ImGui::EndDisabled();
    ImGui::EndChild();
    ImGui::SameLine(0, 16);
    ImGui::BeginChild("##join", ImVec2(half, 330), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PushFont(theme().bold, theme().body_size * 1.2f);
    ImGui::TextUnformatted("Join a race");
    ImGui::PopFont();
    ImGui::SetNextItemWidth(-110);
    ImGui::InputTextWithHint("##code", "Lobby code", np_join_code, sizeof(np_join_code));
    ImGui::SameLine();
    if (ImGui::Button("Join##code", ImVec2(-1, 0)) && np->join && np_join_code[0]) {
        char bind[96] = {0};
        if (np->join(c, np_join_code, np_password, bind) <= 0) call_err("Join failed");
    }
    ImGui::SetNextItemWidth(-110);
    ImGui::InputTextWithHint("##addr", "Address (host:port)", np_address, sizeof(np_address));
    ImGui::SameLine();
    if (ImGui::Button("Join##addr", ImVec2(-1, 0)) && np->join && np_address[0]) {
        char bind[96] = {0};
        const std::string id = std::string("lan:") + np_address;
        const int rc = np->join(c, id.c_str(), np_password, bind);
        if (rc <= 0) call_err(rc == -3 ? "No host answered at that address" : "Join failed");
    }
    if (np->local_address_get) {
        ImGui::TextDisabled("Your addresses:");
        for (int i = 0; i < 4; ++i) {
            RecompLauncherCNetplayLocalAddress a{};
            if (!np->local_address_get(c, i, &a)) break;
            ImGui::TextDisabled("  %s  %s", a.address, a.label);
        }
    }
    ImGui::EndChild();

    if (connected && np->list_count && np->list_get) {
        section("Open lobbies");
        if (ImGui::Button("Refresh") && np->request_list) np->request_list(c);
        const int n = np->list_count(c);
        if (!n) ImGui::TextDisabled("No lobbies right now. Host one!");
        for (int i = 0; i < n; ++i) {
            RecompLauncherCNetplayLobby l{};
            if (!np->list_get(c, i, &l)) continue;
            ImGui::PushID(i);
            char row[200];
            std::snprintf(row, sizeof(row), "%s   %d/%d   %s%d ms", l.name, l.player_count, l.max_slots,
                          l.has_password ? "locked  " : "", l.latency_ms);
            if (ImGui::Selectable(row, false, 0, ImVec2(0, theme().row_h)) && np->join) {
                char bind[96] = {0};
                if (np->join(c, l.lobby_id, np_password, bind) <= 0) call_err("Join failed");
            }
            ImGui::PopID();
        }
    }
}

}  // namespace r4l
