#ifndef APP_HPP
#define APP_HPP

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <string>
#include "discover.hpp"
#include "p2p/tcp_manager.hpp"
#include "common/utils.hpp"
#include "pong.hpp"
#include "shooter.hpp"
#include "imgui.h"

struct Timer {
    Uint64 last_time = 0;

    bool has_elapsed(Uint64 interval_ms) {
        Uint64 now = SDL_GetTicks64();
        if(now - last_time <= interval_ms) return false;
        last_time = now;
        return true;
    }
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
    Discover discover;
    P2P::TcpManager tcp_manager;

    // AppState
    enum AppState {
        AppState_WindowClosed,
        AppState_GameSelect,
        AppState_Pong,
        AppState_Shooter
    };
    AppState app_state = AppState_WindowClosed;
    RoleState role_state = RoleState_Uninitialised;
    Stopwatch frame_stopwatch;

    // Chat State
    std::vector<std::string> messages;
    char chatInput[128] = {0};

    // Game Select
    Game::Type client_vote = Game::Uninitialized;
    Game::Type master_vote = Game::Uninitialized;
    int64_t vote_confirm_countdown = -1;

    // Pong State
    bool paddle_update = false;
    bool proj_update = false;
    Pong pong;

    // Shooter State
    Shooter shooter;

    App(std::string_view name, u_int16_t gamePort):
        discover(Discover::Config(name, gamePort)),
        tcp_manager(P2P::TcpManager::Config(gamePort)) {}
    
    void load_renderer(SDL_Renderer* renderer) {
        shooter.initialize_texture(renderer);
    }
    
    void reset_to_state(AppState _app_state) {
        if (_app_state == AppState_WindowClosed) {
            app_state = AppState_WindowClosed;
            role_state = RoleState_SinglePlayer;
            messages.clear();
        } else if (_app_state == AppState_GameSelect) {
            app_state = AppState_GameSelect;
            vote_confirm_countdown = -1;
            master_vote = Game::Uninitialized;
            client_vote = Game::Uninitialized;
        } else if (_app_state == AppState_Pong) {
            app_state = AppState_Pong;
            pong.reset();
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
            bool is_master = role_state == RoleState_Master;
            auto& paddle = is_master ? pong.rightP : pong.leftP;
            auto& other_paddle = !is_master ? pong.rightP : pong.leftP;
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
        }
    }
    
    void process_events() {
        Logger logger("App");
        discover.process_events();

        static MetaP::Callbacks callbacks(
            [&](const P2P::ConnectResponseBody& connect_response) {
                role_state = connect_response.role_state;
                reset_to_state(AppState_GameSelect);
                logger.log("Connected to ", connect_response.addr, " as ", role_state);
            },
            [&](const P2P::DisconnectResponseBody& disconnect_response) {
                logger.log("Disconnected from ", disconnect_response.addr);
                reset_to_state(AppState_WindowClosed);
            },
            [&](const P2P::MessageBody& message) {
                messages.push_back(std::string("Peer: ") + message.message);
            },
            [&](const P2P::GameVoteBody& game_vote) {
                if (role_state == RoleState_Master) {
                    client_vote = game_vote.type;
                } else {
                    master_vote = game_vote.type;
                }
            },
            [&](const P2P::PongConfigBody& pong_config) {
                pong.unpack(pong_config);
                reset_to_state(AppState_Pong);
            },
            [&](const P2P::PongPaddleBody& pong_paddle) {
                auto& paddle = role_state == RoleState_Master ? pong.leftP : pong.rightP;
                paddle.unpack(pong_paddle);
            },
            [&](const P2P::PongBallBody& pong_ball) {
                pong.ball.unpack(pong_ball);
            }
        );

        P2P::Packet packet;
        while (tcp_manager.incomingQueue.try_pop(packet)) {
            callbacks.dispatch<
                MetaP::TT_TVIsEquals<P2P::TV_BodyType>::type,
                MetaP::TO_VariantCast
            >(packet.type, packet.body);
        }
    }

    void end_process() {
        if (app_state == AppState_WindowClosed) return;
        Uint64 now = SDL_GetTicks64();
        bool is_master = role_state == RoleState_Master;
        if (app_state == AppState_Pong) {
            Logger logger("Pong");
            Pong::State state = pong.step(frame_stopwatch.delta() / 1000.0f);
            switch (state) {
            case Pong::State_Right_Wins:
                messages.push_back("Client won!");
                reset_to_state(AppState_GameSelect);
                break;
            case Pong::State_Left_Wins:
                messages.push_back("Master won!");
                reset_to_state(AppState_GameSelect);
                break;
            default:
                break;
            }
            if (role_state != RoleState_SinglePlayer) {
                if (paddle_update) {
                    auto& paddle = is_master ? pong.rightP : pong.leftP;
                    tcp_manager.outgoingQueue.push(P2P::Packet::create(
                        paddle.pack()
                    ));
                }
                if (is_master) {
                    tcp_manager.outgoingQueue.push(P2P::Packet::create(
                        pong.ball.pack()
                    ));
                }
            }
        } else if (app_state == AppState_GameSelect) {
            if (client_vote == master_vote && master_vote != Game::Uninitialized) {
                if (vote_confirm_countdown < 0) {
                    vote_confirm_countdown = 1000;
                } else if (vote_confirm_countdown > 0) {
                    vote_confirm_countdown -= frame_stopwatch.delta();
                    if (vote_confirm_countdown <= 0) vote_confirm_countdown = 0;

                    if (
                        (is_master || role_state == RoleState_SinglePlayer)
                        && vote_confirm_countdown == 0
                    ) {
                        switch (master_vote) {
                        case Game::Pong: {
                            pong.reset();
                            tcp_manager.outgoingQueue.try_push(P2P::Packet::create(
                                pong.pack()
                            ));
                            reset_to_state(AppState_Pong);
                            break;
                        }
                        case Game::Shooter: 
                            reset_to_state(AppState_Shooter);
                            break;
                        default:
                            break;
                        }
                    }
                }
            } else {
                vote_confirm_countdown = -1;
            }
        }
    }

    void draw_app() {
        if (app_state == AppState_WindowClosed) return;
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
                auto _checkbox = [&](bool master_checkbox, Game::Type game_type, int& id) {
                    bool is_singleplayer = role_state == RoleState_SinglePlayer;
                    bool is_master = role_state == RoleState_Master;
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
                                tcp_manager.outgoingQueue.push(P2P::Packet::create(
                                    P2P::GameVoteBody {
                                        .type = game_type
                                    }
                                ));
                            }
                        } else {
                            if (master_checkbox) {
                                master_vote = Game::Uninitialized;
                            } else {
                                client_vote = Game::Uninitialized;
                            }
                            if (!is_singleplayer) {
                                tcp_manager.outgoingQueue.push(P2P::Packet::create(
                                    P2P::GameVoteBody {
                                        .type = Game::Uninitialized
                                    }
                                ));
                            }
                        }
                    }
                    ImGui::EndDisabled();
                    ImGui::PopID();
                };
                {
                    std::stringstream ss;
                    ss << role_state;
                    ImGui::Text("Role: %s", ss.str().c_str());
                }
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
                    _checkbox(true, Game::Pong, id);
                    ImGui::TableSetColumnIndex(2);
                    _checkbox(false, Game::Pong, id);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Shooter");
                    ImGui::TableSetColumnIndex(1);
                    _checkbox(true, Game::Shooter, id);
                    ImGui::TableSetColumnIndex(2);
                    _checkbox(false, Game::Shooter, id);

                    ImGui::EndTable();
                }
                if (vote_confirm_countdown >= 0) {
                    ImGui::Text("Game starting in %f", vote_confirm_countdown / 1000.0f);
                }
            } else if (app_state == AppState_Pong) {
                pong.draw();
            } else if (app_state == AppState_Shooter) {
                shooter.draw();
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
                if (tcp_manager.sendMessage(chatInput)) {
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
            tcp_manager.disconnect();
            reset_to_state(AppState_WindowClosed);
        }
    }

    void drawDiscover() {
        ImGui::Begin("LAN Users");
        ImGui::Text("User: %s", discover.config.ownLoc.name);
        if (ImGui::BeginTable("neighbour_table", 3, ImGuiTableFlags_Borders, ImVec2(-FLT_MIN, 0.0))) {
            ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Connect", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableHeadersRow();
            {
                for (auto& p: discover.neighbours) {
                    Discover::Loc& neigh = p.second;
                    std::string address;
                    std::string state;
                    bool isHost = neigh.address == discover.config.ownLoc.address;
                    {
                        std::stringstream ss;
                        ss << neigh;
                        address = ss.str();
                        ss.str("");
                        if (isHost) {
                            ss << Discover::Loc::IsHost;
                        } else {
                            ss << neigh.state;
                        }
                        state = ss.str();
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", address.data());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", state.data());
                    ImGui::TableSetColumnIndex(2);
                    float cellWidth = ImGui::GetContentRegionAvail().x;
                    ImGui::PushID(address.data());
                    ImGui::BeginDisabled(neigh.state != Discover::Loc::Available);
                    if (ImGui::Button("Chat", ImVec2{cellWidth, 20.0f})) {
                        if (isHost) {
                            role_state = RoleState_SinglePlayer;
                            reset_to_state(AppState_GameSelect);
                        } else {
                            tcp_manager.connect(neigh.address);
                        }
                    }
                    ImGui::EndDisabled();
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }
};

#endif