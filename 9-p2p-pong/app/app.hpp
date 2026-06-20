#ifndef APP_HPP
#define APP_HPP

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <string>
#include "imgui.h"
#include "lib/common/utils.hpp"
#include "discover/discover.hpp"
#include "p2p/p2p.hpp"
#include "pong/pong.hpp"
#include "shooter/shooter.hpp"
#include "game_select/game_select.hpp"
#include "chat/chat.hpp"

namespace App {
    enum AppState {
        AppState_WindowClosed,
        AppState_GameSelect,
        AppState_Pong,
        AppState_Shooter
    };

    struct Stopwatch {
        Uint64 last_time = 0;
        Uint64 now = 0;
        void step() {
            last_time = now;
            now = SDL_GetTicks64();
        }

        Uint64 delta() {
            if (last_time == 0) {
                return 0;
            } else {
                return now - last_time;
            }
        }
    };

    struct App {
        // AppState
        AppState app_state = AppState_WindowClosed;
        P2P::RoleType role_type = P2P::RoleType_Uninitialised;
        bool is_initialised = false;
        Stopwatch frame_stopwatch;

        bool initialise(std::string_view name, u_int16_t gamePort, SDL_Renderer* renderer) {
            if (is_initialised) return false;
            Discover::discover.initialise(Discover::Config(name, gamePort));
            P2P::tcp_manager.initialise(P2P::TcpManager::Config(gamePort));
            Shooter::shooter.initialise(renderer);
            return true;
        }
        
        
        void reset_to_state(AppState _app_state) {
            if (_app_state == AppState_WindowClosed) {
                app_state = AppState_WindowClosed;
                Chat::chat.reset();
            } else if (_app_state == AppState_GameSelect) {
                app_state = AppState_GameSelect;
                GameSelect::game_select.reset();
            } else if (_app_state == AppState_Pong) {
                app_state = AppState_Pong;
                Pong::pong.reset();
                if (role_type == P2P::RoleType_Master) {
                    P2P::tcp_manager.outgoingQueue.try_push(P2P::Packet::create(
                        Pong::pong.pack()
                    ));
                }
            } else if (_app_state == AppState_Shooter) {
                app_state = AppState_Shooter;
            }
        }
        
        void process_setup() {
            if (app_state == AppState_WindowClosed) return;
            frame_stopwatch.step();
            Chat::chat.process_setup();
            Pong::pong.process_setup();
            GameSelect::game_select.process_setup();
        }
        
        void process_sdl_events(const SDL_Event& event) {
            if (app_state == AppState_Pong) {
                Pong::pong.process_sdl_event(event);
            } else if (app_state == AppState_GameSelect) {
                GameSelect::game_select.process_sdl_event(event);
            } else if (app_state == AppState_Shooter) {
                Shooter::shooter.process_sdl_event(event);
            }
        }
        
        void process_events() {
            Logger logger("App");
            Discover::discover.process_events();

            static MetaP::Callbacks callbacks(
                [&](const P2P::ConnectResponseBody& connect_response) {
                    role_type = connect_response.role_type;
                    reset_to_state(AppState_GameSelect);
                    logger.log("Connected to ", connect_response.addr, " as ", role_type);
                },
                [&](const P2P::DisconnectResponseBody& disconnect_response) {
                    logger.log("Disconnected from ", disconnect_response.addr);
                    reset_to_state(AppState_WindowClosed);
                }
            );

            P2P::Packet packet;
            while (P2P::tcp_manager.incomingQueue.try_pop(packet)) {
                callbacks.dispatch<
                    MetaP::TT_TVIsEquals<P2P::TV_BodyType>::type,
                    MetaP::TO_VariantCast
                >(packet.type, packet.body);
                GameSelect::game_select.process_packet(packet);
                Chat::chat.process_packet(packet);
                Pong::pong.process_packet(packet);
            }
        }

        void draw() {
            if (app_state == AppState_WindowClosed) {
                Discover::discover.draw();
            } else {
                bool isOpen = true;
                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                if (ImGui::Begin("Game", &isOpen)) {
                    ImVec2 windowSize = ImGui::GetContentRegionAvail();
                    float rightPanelWidth = 200.0f; // Or windowSize.x * 0.3f for percentage
                    float leftPanelWidth = windowSize.x - 200.0f; // Or windowSize.x * 0.3f for percentage
                    ImGui::BeginChild("LeftPanel", ImVec2(leftPanelWidth, 0), true);

                    if (app_state == AppState_GameSelect) {
                        GameSelect::game_select.draw();
                    } else if (app_state == AppState_Pong) {
                        Pong::pong.draw();
                    } else if (app_state == AppState_Shooter) {
                        Shooter::shooter.draw();
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();
                    ImGui::BeginChild("RightPanel", ImVec2(rightPanelWidth, 0), true);
                    Chat::chat.draw();
                    ImGui::EndChild();
                }
                ImGui::End();

                if (!isOpen) {
                    P2P::tcp_manager.disconnect();
                    reset_to_state(AppState_WindowClosed);
                }
            }
        }
    };

    extern App app;
}

#endif