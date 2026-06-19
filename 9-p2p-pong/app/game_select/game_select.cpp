#include "game_select.hpp"

#include "p2p/tcp_manager.hpp"
#include "app/app.hpp"
#include "app/pong.hpp"

namespace GameSelect {
    GameSelect game_select;

    void GameSelect::draw_checkbox(
        P2P::RoleType role_type,
        bool master_checkbox, 
        P2P::GameType game_type,
        int& id
    ) {
        bool is_singleplayer = role_type == P2P::RoleType_SinglePlayer;
        bool is_master = role_type == P2P::RoleType_Master;
        ImGui::PushID(++id);
        ImGui::BeginDisabled(!is_singleplayer && is_master != master_checkbox);
        bool checked = (master_checkbox ? master_vote : client_vote) == game_type;
        if (ImGui::Checkbox("Vote", &checked)) {
            if (checked) {
                if (master_checkbox) {
                    master_vote = game_type;
                } else {
                    client_vote = game_type;
                }
                if (!is_singleplayer) {
                    P2P::tcp_manager.outgoingQueue.push(P2P::Packet::create(
                        P2P::GameVoteBody {
                            .type = game_type
                        }
                    ));
                }
            } else {
                if (master_checkbox) {
                    master_vote = P2P::GameType_Uninitialized;
                } else {
                    client_vote = P2P::GameType_Uninitialized;
                }
                if (!is_singleplayer) {
                    P2P::tcp_manager.outgoingQueue.push(P2P::Packet::create(
                        P2P::GameVoteBody {
                            .type = P2P::GameType_Uninitialized
                        }
                    ));
                }
            }
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    void GameSelect::draw(P2P::RoleType role_type, Pong& pong_ref) {
        ImGui::Text("Role: %s", P2P::to_string(role_type).c_str());
        ImGui::Separator();
        if (ImGui::BeginTable("GameSelectTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Game");
            ImGui::TableSetupColumn("Master Vote");
            ImGui::TableSetupColumn("Client Vote");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Pong");
            ImGui::TableSetColumnIndex(1);
            int id = 0;
            draw_checkbox(role_type, true, P2P::GameType_Pong, id);
            ImGui::TableSetColumnIndex(2);
            draw_checkbox(role_type, false, P2P::GameType_Pong, id);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Shooter");
            ImGui::TableSetColumnIndex(1);
            draw_checkbox(role_type, true, P2P::GameType_Shooter, id);
            ImGui::TableSetColumnIndex(2);
            draw_checkbox(role_type, false, P2P::GameType_Shooter, id);

            ImGui::EndTable();
        }
        if (vote_confirm_countdown >= 0) {
            ImGui::Text("Game starting in %f", vote_confirm_countdown / 1000.0f);
        }

        bool is_resolved = client_vote == master_vote && master_vote != P2P::GameType_Uninitialized;
        if (!is_resolved) {
            vote_confirm_countdown = -1;
        } else if (vote_confirm_countdown < 0) {
            vote_confirm_countdown = 1000;
        } else if (vote_confirm_countdown > 0) {
            vote_confirm_countdown -= App::app.frame_stopwatch.delta();

            bool is_master = (
                role_type == P2P::RoleType_Master || 
                role_type == P2P::RoleType_SinglePlayer
            );
            bool is_send = (
                role_type == P2P::RoleType_Master
            );
            if (is_master && vote_confirm_countdown <= 0) {
                switch (master_vote) {
                case P2P::GameType_Pong: {
                    pong_ref.reset();
                    if (is_send) {
                        P2P::tcp_manager.outgoingQueue.try_push(P2P::Packet::create(
                            pong_ref.pack()
                        ));
                    }
                    App::app.reset_to_state(App::AppState_Pong);
                    break;
                }
                case P2P::GameType_Shooter: 
                    App::app.reset_to_state(App::AppState_Shooter);
                    break;
                default:
                    break;
                }
            }
        }
    }
}