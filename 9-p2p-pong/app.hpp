#ifndef APP_HPP
#define APP_HPP

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <string>
#include "discover.hpp"
#include "p2pTcp.hpp"
#include "utils.hpp"
#include "pong.hpp"
#include "imgui.h"

struct App {
    Discover discover;
    TcpManager manager;


    // AppState
    enum AppState {
        AppState_WindowClosed,
        AppState_GameSelect,
        AppState_ReadyMenu,
        AppState_Pong,
    };
    AppState app_state = AppState_WindowClosed;
    bool is_master = false;

    // Chat State
    std::vector<std::string> messages;
    char chatInput[128] = {0};

    // Pong State
    enum Winner {
        Winner_None,
        Winner_Master,
        Winner_Client,
    };
    Winner winner = Winner_None;
    Uint64 last_frame_ms = 0;
    Uint64 last_ball_update_ms = 0;
    Uint64 delta_ms = 0;
    bool master_ready = false;
    bool client_ready = false;
    bool paddle_update = false;
    Pong pong;

    App(std::string_view name, u_int16_t gamePort):
        discover(Discover::Config(name, gamePort)),
        manager(TcpManager::Config(gamePort)) {}
    
    void reset_to_state(AppState _app_state) {
        if (_app_state == AppState_ReadyMenu) {
            app_state = AppState_ReadyMenu;
            last_frame_ms = 0;
            delta_ms = 0;
            master_ready = false;
            client_ready = false;
        } else if (_app_state == AppState_WindowClosed) {
            app_state = AppState_WindowClosed;
            last_frame_ms = 0;
            delta_ms = 0;
            master_ready = false;
            client_ready = false;
            messages.clear();
            winner = Winner_None;
        } else if (_app_state == AppState_Pong) {
            app_state = AppState_Pong;
        }
    }
    
    void begin_process() {
        if (app_state == AppState_WindowClosed) return;
        paddle_update = false;
    }
    
    void process_sdl_events(const SDL_Event& event) {
        if (app_state != AppState_Pong) return;
        auto& paddle = is_master ? pong.rightP : pong.leftP;
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
                    default:
                        break;
                }
                break;
            }
            case SDL_KEYUP: {
                auto key = event.key.keysym.sym;
                switch (key) {
                    case SDLK_w:
                    case SDLK_s:
                        paddle.move(0);
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
    
    void process_events() {
        Logger logger("App");
        discover.process_events();

        static MetaP::Callbacks callbacks(
            [&](const Packet::ConnectResponseBody& connect_response) {
                is_master = connect_response.is_master;
                app_state = AppState_ReadyMenu;
                winner = Winner_Master;
                reset_to_state(AppState_ReadyMenu);
                logger.log("Connected to ", connect_response.addr, " as ", is_master ? "Master": "Client");
            },
            [&](const Packet::DisconnectResponseBody& disconnect_response) {
                logger.log("Disconnected from ", disconnect_response.addr);
                reset_to_state(AppState_WindowClosed);
            },
            [&](const Packet::MessageBody& message) {
                messages.push_back(std::string("Peer: ") + message.message);
            },
            [&](const Packet::PongConfigBody& pong_config) {
                pong.unpack(pong_config);
                reset_to_state(AppState_Pong);
            },
            [&](const Packet::PongReadyBody& pong_ready) {
                if (is_master) {
                    client_ready = pong_ready.ready;
                } else {
                    master_ready = pong_ready.ready;
                }
            },
            [&](const Packet::PongPaddleBody& pong_paddle) {
                auto& paddle = is_master ? pong.leftP : pong.rightP;
                paddle.unpack(pong_paddle);
            },
            [&](const Packet::PongBallBody& pong_ball) {
                pong.ball.unpack(pong_ball);
            }
        );

        Packet::Packet packet;
        while (manager.incomingQueue.try_pop(packet)) {
            callbacks.dispatch<
                MetaP::TT_TVIsEquals<Packet::TV_BodyType>::type,
                MetaP::TO_VariantCast
            >(packet.type, packet.body);
        }
    }

    void end_process() {
        if (app_state == AppState_WindowClosed) return;
        Uint64 now = SDL_GetTicks64();
        if (app_state == AppState_Pong) {
            Logger logger("Pong");
            delta_ms = last_frame_ms == 0 
                ? 0
                : now - last_frame_ms;
            last_frame_ms = now;
            Pong::State state = pong.step(delta_ms / 1000.0f);
            switch (state) {
            case Pong::State_Right_Wins:
                winner = Winner_Client;
                reset_to_state(AppState_ReadyMenu);
                break;
            case Pong::State_Left_Wins:
                winner = Winner_Master;
                reset_to_state(AppState_ReadyMenu);
                break;
            default:
                break;
            }
        }
        if (paddle_update) {
            auto& paddle = is_master ? pong.rightP : pong.leftP;
            manager.outgoingQueue.push(Packet::Packet::create(
                paddle.pack()
            ));
        }
        if (is_master && now - last_ball_update_ms > 100) {
            last_ball_update_ms = now;
            manager.outgoingQueue.push(Packet::Packet::create(
                pong.ball.pack()
            ));
        }
    }

    void draw_app() {
        if (app_state == AppState_WindowClosed) return;
        bool isOpen = true;
        if (ImGui::Begin("Game", &isOpen)) {
            ImVec2 windowSize = ImGui::GetContentRegionAvail();
            float rightPanelWidth = 200.0f; // Or windowSize.x * 0.3f for percentage
            float leftPanelWidth = windowSize.x - 200.0f; // Or windowSize.x * 0.3f for percentage
            ImGui::BeginChild("LeftPanel", ImVec2(leftPanelWidth, 0), true);

            ImGui::Text("Role: %s", is_master ? "Master" : "Client");
            ImGui::Text("Controls: W / S");
            ImGui::Separator();

            if (app_state == AppState_ReadyMenu) {
                if (winner != Winner_None) {
                    ImGui::Text("Winner is %s", winner == Winner_Master ? "Master" : "Client");
                }
                ImGui::Text("Ready Status:");
                ImGui::Separator();
                
                // Table showing both players' ready status
                if (ImGui::BeginTable("ReadyTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Player");
                    ImGui::TableSetupColumn("Ready");
                    ImGui::TableHeadersRow();
                    
                    // Master row
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Master");
                    ImGui::TableNextColumn();
                    if (master_ready) {
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Ready");
                    } else {
                        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "Not Ready");
                    }
                    
                    // Client row
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Client");
                    ImGui::TableNextColumn();
                    if (client_ready) {
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Ready");
                    } else {
                        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "Not Ready");
                    }
                    
                    ImGui::EndTable();
                }
                
                ImGui::Spacing();
                
                // Local ready checkbox
                bool my_ready_value = is_master ? master_ready : client_ready;
                if (ImGui::Checkbox("I'm Ready!", &my_ready_value)) {
                    // Update local state
                    if (is_master) {
                        master_ready = my_ready_value;
                    } else {
                        client_ready = my_ready_value;
                    }
                    manager.outgoingQueue.try_push(Packet::Packet::create(
                        Packet::PongReadyBody{
                            .ready = my_ready_value
                        }
                    ));
                }
                
                // Start game when both ready
                if (master_ready && client_ready) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Both players ready!");
                    
                    // Only master can start the game
                    if (is_master) {
                        if (ImGui::Button("Start Game")) {
                            // Reset and pack game state
                            pong.reset();
                            manager.outgoingQueue.try_push(Packet::Packet::create(
                                pong.pack()
                            ));
                            
                            // Start game locally
                            reset_to_state(AppState_Pong);
                            last_frame_ms = SDL_GetTicks64();
                        }
                    } else {
                        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Waiting for master to start...");
                    }
                }

            } else if (app_state == AppState_Pong) {
                pong.draw();
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
                if (manager.sendMessage(chatInput)) {
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
            manager.disconnect();
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
                    ImGui::BeginDisabled(isHost || neigh.state != Discover::Loc::Available);
                    if (ImGui::Button("Chat", ImVec2{cellWidth, 20.0f})) {
                        manager.connect(neigh.address);
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