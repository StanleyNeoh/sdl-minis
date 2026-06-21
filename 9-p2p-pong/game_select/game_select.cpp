#include "game_select.hpp"

#include "p2p/tcp_manager.hpp"
#include "lib/metap/metap.hpp"
#include "app/app.hpp"
#include "chat/chat.hpp"
#include "pong/pong.hpp"
#include "SDL.h"

namespace GameSelect {
    GameSelect game_select;

    bool is_singleplayer() {
        return App::app.role_type == P2P::RoleType_SinglePlayer;
    }

    void GameSelect::process_sdl_event(const SDL_Event& event) {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        switch (event.type) {
            case SDL_KEYDOWN: {
                auto key = event.key.keysym.sym;
                switch (key) {
                    case SDLK_1: {
                        if (state[SDL_SCANCODE_G]) {
                            Pong::pong.leftScore = 0;
                            Pong::pong.rightScore = 0;
                            Chat::chat.messages.push_back("Pong score resetted!");
                        } else {
                            App::app.reset_to_state(App::AppState_Pong);
                        }
                        break;
                    }
                    case SDLK_2: {
                        if (state[SDL_SCANCODE_G]) {
                            Shooter::shooter.ship1_score = 0;
                            Shooter::shooter.ship2_score = 0;
                            Chat::chat.messages.push_back("Shooter score resetted!");
                        } else {
                            App::app.reset_to_state(App::AppState_Shooter);
                        }
                        break;
                    }
                }
                break;
            }
            default:
                break;
        }
    };

    void GameSelect::process_packet(P2P::Packet& packet) {
        static MetaP::Callbacks callbacks(
            [&](const P2P::GameVoteBody& game_vote) {
                other_vote = game_vote.type;
            }
        );
        callbacks.dispatch<
            MetaP::TT_TVIsEquals<P2P::TV_BodyType>::type,
            MetaP::TO_VariantCast
        >(packet.type, packet.body);
    }


    void GameSelect::draw_checkbox(
        std::string_view id,
        P2P::GameType& vote,
        P2P::GameType game_type,
        bool mine
    ) {
        ImGui::PushID(id.data());
        ImGui::BeginDisabled(!mine);
        bool checked = vote == game_type || (!mine && is_singleplayer());
        std::string name = mine ? "Mine": "Other";
        if (ImGui::Checkbox(name.data(), &checked)) {
            vote = checked ? game_type : P2P::GameType_Uninitialized;
            if (!is_singleplayer()) {
                P2P::tcp_manager.outgoingQueue.push(P2P::Packet::create(
                    P2P::GameVoteBody {
                        .type = vote
                    }
                ));
            }
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    void GameSelect::draw() {
        P2P::RoleType& role_type = App::app.role_type;

        ImGui::Text("Role: %s", P2P::to_string(role_type).c_str());
        ImGui::Separator();
        if (ImGui::BeginTable("GameSelectTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Game");
            ImGui::TableSetupColumn("Vote");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Pong");
            ImGui::TableSetColumnIndex(1);
            draw_checkbox("pong_mine", my_vote, P2P::GameType_Pong, true);
            ImGui::SameLine();
            draw_checkbox("pong_other", other_vote, P2P::GameType_Pong, false);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Shooter");
            ImGui::TableSetColumnIndex(1);
            draw_checkbox("shooter_other", my_vote, P2P::GameType_Shooter, true);
            ImGui::SameLine();
            draw_checkbox("shooter_other", other_vote, P2P::GameType_Shooter, false);

            ImGui::EndTable();
        }
        if (vote_confirm_countdown >= 0) {
            ImGui::Text("Game starting in %f", vote_confirm_countdown / 1000.0f);
        }

        bool is_resolved = my_vote != P2P::GameType_Uninitialized && (my_vote == other_vote || is_singleplayer());
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
            if (is_master && vote_confirm_countdown <= 0) {
                switch (my_vote) {
                case P2P::GameType_Pong: {
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