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

        // Chat State
        std::vector<std::string> messages;
        char chatInput[128] = {0};

        // Pong State
        bool paddle_update = false;
        bool proj_update = false;

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
                role_type = P2P::RoleType_SinglePlayer;
                messages.clear();
            } else if (_app_state == AppState_GameSelect) {
                app_state = AppState_GameSelect;
                GameSelect::game_select.reset();
            } else if (_app_state == AppState_Pong) {
                app_state = AppState_Pong;
                Pong::pong.reset();
            } else if (_app_state == AppState_Shooter) {
                app_state = AppState_Shooter;
            }
        }
        
        void begin_process() {
            if (app_state == AppState_WindowClosed) return;
            frame_stopwatch.step();
            paddle_update = false;
            proj_update = false;
        }
        
        void process_sdl_events(const SDL_Event& event) {
            if (app_state == AppState_Pong) {
                bool is_master = role_type == P2P::RoleType_Master;
                auto& paddle = is_master ? Pong::pong.rightP : Pong::pong.leftP;
                auto& other_paddle = !is_master ? Pong::pong.rightP : Pong::pong.leftP;
                switch (event.type) {
                    case SDL_KEYDOWN: {
                        auto key = event.key.keysym.sym;
                        switch (key) {
                            case SDLK_w:
                                paddle.move(-20.0);
                                paddle_update = true;
                                break;
                            case SDLK_s:
                                paddle.move(20.0);
                                paddle_update = true;
                                break;
                            case SDLK_UP:
                                other_paddle.move(-20.0);
                                paddle_update = true;
                                break;
                            case SDLK_DOWN:
                                other_paddle.move(20.0);
                                paddle_update = true;
                                break;
                            case SDLK_SPACE:
                                paddle.shoot(!is_master);
                                paddle_update = true;
                                break;
                            case SDLK_RSHIFT:
                                other_paddle.shoot(is_master);
                                paddle_update = true;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    case SDL_KEYUP: {
                        auto key = event.key.keysym.sym;
                        const Uint8* state = SDL_GetKeyboardState(NULL);
                        switch (key) {
                            case SDLK_w:
                            case SDLK_s: {
                                if (state[SDL_SCANCODE_W] || state[SDL_SCANCODE_S]) break;
                                paddle.move(0);
                                paddle_update = true;
                                break;
                            }
                            case SDLK_UP:
                            case SDLK_DOWN:
                                if (state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_DOWN]) break;
                                other_paddle.move(0);
                                paddle_update = true;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    default:
                        break;
                }
            } else if (app_state == AppState_GameSelect) {
                // Hacky quick start pong with spacebar
                switch (event.type) {
                    case SDL_KEYDOWN: {
                        auto key = event.key.keysym.sym;
                        switch (key) {
                            case SDLK_SPACE: {
                                reset_to_state(AppState_Pong);
                                break;
                            }
                            case SDLK_g: {
                                Pong::pong.clientScore = 0;
                                Pong::pong.masterScore = 0;
                                messages.push_back("Score resetted!");
                                break;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
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
                },
                [&](const P2P::MessageBody& message) {
                    messages.push_back(std::string("Peer: ") + message.message);
                },
                [&](const P2P::GameVoteBody& game_vote) {
                    if (role_type == P2P::RoleType_Master) {
                        GameSelect::game_select.client_vote = game_vote.type;
                    } else {
                        GameSelect::game_select.master_vote = game_vote.type;
                    }
                },
                [&](const P2P::PongConfigBody& pong_config) {
                    Pong::pong.unpack(pong_config);
                    reset_to_state(AppState_Pong);
                },
                [&](const P2P::PongPaddleBody& pong_paddle) {
                    auto& paddle = role_type == P2P::RoleType_Master ? Pong::pong.leftP : Pong::pong.rightP;
                    paddle.unpack(pong_paddle);
                },
                [&](const P2P::PongBallBody& pong_ball) {
                    Pong::pong.ball.unpack(pong_ball);
                }
            );

            P2P::Packet packet;
            while (P2P::tcp_manager.incomingQueue.try_pop(packet)) {
                callbacks.dispatch<
                    MetaP::TT_TVIsEquals<P2P::TV_BodyType>::type,
                    MetaP::TO_VariantCast
                >(packet.type, packet.body);
            }
        }

        void end_process() {
            if (app_state == AppState_WindowClosed) return;
            Uint64 now = SDL_GetTicks64();
            bool is_master = role_type == P2P::RoleType_Master;
            if (app_state == AppState_Pong) {
                Logger logger("Pong");
                Pong::State state = Pong::pong.step(frame_stopwatch.delta() / 1000.0f);
                switch (state) {
                case Pong::State_Right_Wins:
                    Pong::pong.clientScore++;
                    messages.push_back("Client won!");
                    messages.push_back("Score= " + std::to_string(Pong::pong.clientScore) + " : " + std::to_string(Pong::pong.masterScore));
                    reset_to_state(AppState_GameSelect);
                    break;
                case Pong::State_Left_Wins:
                    Pong::pong.masterScore++;
                    messages.push_back("Master won!");
                    messages.push_back("Score= " + std::to_string(Pong::pong.clientScore) + " : " + std::to_string(Pong::pong.masterScore));
                    reset_to_state(AppState_GameSelect);
                    break;
                default:
                    break;
                }
                if (role_type != P2P::RoleType_SinglePlayer) {
                    if (paddle_update) {
                        auto& paddle = is_master ? Pong::pong.rightP : Pong::pong.leftP;
                        P2P::tcp_manager.outgoingQueue.push(P2P::Packet::create(
                            paddle.pack()
                        ));
                    }
                    if (is_master) {
                        P2P::tcp_manager.outgoingQueue.push(P2P::Packet::create(
                            Pong::pong.ball.pack()
                        ));
                    }
                }
            } else if (app_state == AppState_GameSelect) {
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

                    ImGui::SeparatorText("Chat");
                    if (ImGui::BeginChild("chat_messages", ImVec2(0.0f, 180.0f), ImGuiChildFlags_Borders)) {
                        for (const auto& message: messages) {
                            ImGui::TextWrapped("%s", message.c_str());
                        }
                    }
                    ImGui::EndChild();

                    bool sendChat = ImGui::InputText("##chat_input", chatInput, sizeof(chatInput), ImGuiInputTextFlags_EnterReturnsTrue);
                    ImGui::SameLine();
                    sendChat = ImGui::Button("Send") || sendChat;
                    if (sendChat && chatInput[0] != '\0') {
                        if (P2P::tcp_manager.sendMessage(chatInput)) {
                            messages.push_back(std::string("Me: ") + chatInput);
                            chatInput[0] = '\0';
                        } else {
                            messages.push_back("Failed to send: no active TCP connection");
                        }
                    }
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