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
        AppState_ReadyMenu,
        AppState_Ongoing,
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
        } else if (_app_state == AppState_Ongoing) {
            app_state = AppState_Ongoing;
        }
    }
    
    void begin_process() {
        if (app_state == AppState_WindowClosed) return;
        paddle_update = false;
    }
    
    void process_sdl_events(const SDL_Event& event) {
        if (app_state != AppState_Ongoing) return;
        auto& paddle = is_master ? pong.botP : pong.topP;
        switch (event.type) {
            case SDL_KEYDOWN: {
                auto key = event.key.keysym.sym;
                switch (key) {
                    case SDLK_a:
                        paddle.move(-20.0);
                        paddle_update = true;
                        break;
                    case SDLK_d:
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
                    case SDLK_a:
                    case SDLK_d:
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

        Packet::Packet packet;
        while (manager.incomingQueue.try_pop(packet)) {
            switch (packet.type) {
                case Packet::ConnectResponseType: {
                    is_master = packet.data.connect_response.is_master;
                    app_state = AppState_ReadyMenu;
                    winner = Winner_Master;
                    reset_to_state(AppState_ReadyMenu);
                    logger.log("Connected to ", packet.data.connect_response.addr, " as ", is_master ? "Master": "Client");
                    break;
                }
                case Packet::DisconnectResponseType: {
                    logger.log("Disconnected from ", packet.data.disconnect_response.addr);
                    reset_to_state(AppState_WindowClosed);
                    break;
                }
                case Packet::MessageType:
                    messages.push_back(std::string("Peer: ") + packet.data.message.message);
                    break;
                case Packet::PongConfigType: {
                    pong.unpack(packet.data.pong_config);
                    reset_to_state(AppState_Ongoing);
                    break;
                }
                case Packet::PongReadyType: {
                    if (is_master) {
                        client_ready = packet.data.pong_ready.ready;
                    } else {
                        master_ready = packet.data.pong_ready.ready;
                    }
                    break;
                }
                case Packet::PongPaddleType: {
                    auto& paddle = is_master ? pong.topP : pong.botP;
                    paddle.unpack(packet.data.pong_paddle);
                    break;
                }
                case Packet::PongBallType: {
                    pong.ball.unpack(packet.data.pong_ball);
                    break;
                }
                default:
                    break;
            }
        }
    }

    void end_process() {
        if (app_state == AppState_WindowClosed) return;
        Uint64 now = SDL_GetTicks64();
        if (app_state == AppState_Ongoing) {
            Logger logger("Pong");
            delta_ms = last_frame_ms == 0 
                ? 0
                : now - last_frame_ms;
            last_frame_ms = now;
            Pong::State state = pong.step(delta_ms / 1000.0f);
            switch (state) {
            case Pong::State_Top_Wins:
                winner = Winner_Client;
                reset_to_state(AppState_ReadyMenu);
                break;
            case Pong::State_Bot_Wins:
                winner = Winner_Master;
                reset_to_state(AppState_ReadyMenu);
                break;
            default:
                break;
            }
        }
        if (paddle_update) {
            auto& paddle = is_master ? pong.botP : pong.topP;
            Packet::Packet packet{
                .type = Packet::PongPaddleType,
                .data = { 
                    .pong_paddle = paddle.pack()
                } 
            };
            manager.outgoingQueue.push(packet);
        }
        if (is_master && now - last_ball_update_ms > 100) {
            last_ball_update_ms = now;
            Packet::Packet packet{
                .type = Packet::PongBallType,
                .data = { 
                    .pong_ball = pong.ball.pack()
                } 
            };
            manager.outgoingQueue.push(packet);
        }
    }

    void draw_app() {
        if (app_state != AppState_WindowClosed) {
            bool isOpen = true;
            if (ImGui::Begin("Game", &isOpen)) {
                ImVec2 windowSize = ImGui::GetContentRegionAvail();
                float leftPanelWidth = 200.0f; // Or windowSize.x * 0.3f for percentage
                ImGui::BeginChild("LeftPanel", ImVec2(leftPanelWidth, 0), true);

                ImGui::Text("Role: %s", is_master ? "Master" : "Client");
                ImGui::Text("Controls: A / D");
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
                        
                        // Send ready state to peer
                        Packet::Packet packet{
                            .type = Packet::PongReadyType,
                            .data = { .pong_ready = { .ready = my_ready_value } }
                        };
                        manager.outgoingQueue.try_push(packet);
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
                                
                                // Send config to peer (this signals game start)
                                Packet::Packet packet{
                                    .type = Packet::PongConfigType,
                                    .data = { .pong_config = pong.pack() }
                                };
                                manager.outgoingQueue.try_push(packet);
                                
                                // Start game locally
                                reset_to_state(AppState_Ongoing);
                                last_frame_ms = SDL_GetTicks64();
                            }
                        } else {
                            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Waiting for master to start...");
                        }
                    }

                } else if (app_state == AppState_Ongoing) {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    float canvasSide = std::max(200.0f, std::min(avail.x, avail.y));
                    ImVec2 canvasSize(canvasSide, canvasSide);
                    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                    ImGui::InvisibleButton("pong_canvas", canvasSize);

                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    const float padding = 12.0f;
                    ImVec2 boardMin(canvasPos.x + padding, canvasPos.y + padding);
                    ImVec2 boardMax(canvasPos.x + canvasSize.x - padding, canvasPos.y + canvasSize.y - padding);
                    float boardWidth = boardMax.x - boardMin.x;
                    float boardHeight = boardMax.y - boardMin.y;
                    float scaleX = boardWidth / pong.width;
                    float scaleY = boardHeight / pong.height;

                    auto world_to_screen = [&](float x, float y) {
                        return ImVec2(boardMin.x + x * scaleX, boardMin.y + y * scaleY);
                    };

                    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(18, 18, 18, 255), 8.0f);
                    drawList->AddRect(boardMin, boardMax, IM_COL32(220, 220, 220, 255), 4.0f, 0, 2.0f);

                    ImVec2 centerTop = world_to_screen(pong.width * 0.5f, 0.0f);
                    ImVec2 centerBottom = world_to_screen(pong.width * 0.5f, pong.height);
                    drawList->AddLine(centerTop, centerBottom, IM_COL32(90, 90, 90, 255), 1.0f);

                    auto draw_paddle = [&](const Pong::Paddle& paddle, ImU32 color) {
                        ImVec2 paddleMin = world_to_screen(paddle.pos.x, paddle.pos.y - 0.35f);
                        ImVec2 paddleMax = world_to_screen(paddle.pos.x + paddle.w, paddle.pos.y + 0.35f);
                        drawList->AddRectFilled(paddleMin, paddleMax, color, 4.0f);
                    };

                    draw_paddle(pong.topP, IM_COL32(104, 211, 145, 255));
                    draw_paddle(pong.botP, IM_COL32(95, 145, 255, 255));

                    ImVec2 ballPos = world_to_screen(pong.ball.pos.x, pong.ball.pos.y);
                    float ballRadius = std::max(4.0f, pong.ball.r * 0.5f * (scaleX + scaleY));
                    drawList->AddCircleFilled(ballPos, ballRadius, IM_COL32(255, 244, 214, 255), 24);
                }
                ImGui::EndChild();

                ImGui::SameLine();
                ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);

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