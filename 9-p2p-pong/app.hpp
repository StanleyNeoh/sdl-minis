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

    // Chat State
    bool chatWindowOpen = false;
    std::vector<std::string> messages;
    char chatInput[128] = {0};

    // Pong State
    bool pongUpdate = false;
    bool pongWindowOpen = false;
    bool isMaster = false;
    Uint64 lastFrameTick = 0;
    Pong pong;

    static constexpr float PongBallSpeedMultiplier = 12.0f;

    App(std::string_view name, u_int16_t gamePort):
        discover(Discover::Config(name, gamePort)),
        manager(TcpManager::Config(gamePort)) {}
    
    void begin_process() {
        pongUpdate = false;
        Uint64 now = SDL_GetTicks64();
        float deltaSeconds = 0.0f;
        if (lastFrameTick != 0) {
            deltaSeconds = static_cast<float>(now - lastFrameTick) / 1000.0f;
        }
        lastFrameTick = now;

        if (!pongWindowOpen || !isMaster || deltaSeconds <= 0.0f) return;

        Pong::State state = pong.step(deltaSeconds * PongBallSpeedMultiplier);
        if (state != Pong::State_Ongoing) {
            pong.reset();
        }
        pongUpdate = true;
    }
    
    void process_sdl_events(const SDL_Event& event) {
        if (!pongWindowOpen) return;
        switch (event.type) {
            case SDL_KEYDOWN: {
                auto key = event.key.keysym.sym;
                if (isMaster) {
                    switch (key) {
                        case SDLK_a:
                            pong.botP.move(-0.5, 0.0f, pong.width);
                            pongUpdate = true;
                            break;
                        case SDLK_d:
                            pong.botP.move(0.5, 0.0f, pong.width);
                            pongUpdate = true;
                            break;
                        default:
                            break;
                    }
                } else {
                    switch(key) {
                        case SDLK_a:
                        case SDLK_d:
                            manager.outgoingQueue.push(TcpManager::Event{
                                .type=TcpManager::Event::SDLEvent,
                                .data = { .sdlEvent = {
                                    .event = event
                                }}
                            });
                            break;
                        default:
                            break;
                    }
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

        TcpManager::Event event;
        while (manager.incomingQueue.try_pop(event)) {
            switch (event.type) {
                case TcpManager::Event::ConnectEvent: {
                    if (event.data.connectEvent.success) {
                        pongWindowOpen = true;
                        chatWindowOpen = true;
                        isMaster = event.data.connectEvent.isMaster;
                        pong.reset();
                        lastFrameTick = 0;
                        logger.log("Connected to ", event.data.connectEvent.addr, " as ", isMaster ? "Master": "Client");
                    } else {
                        chatWindowOpen = false;
                        pongWindowOpen = false;
                    }
                    break;
                }
                case TcpManager::Event::DisconnectEvent: {
                    if (!event.data.disconnectEvent.success) break;
                    logger.log("Disconnected from ", event.data.disconnectEvent.addr);
                    messages.clear();
                    chatWindowOpen = false;
                    pongWindowOpen = false;
                    lastFrameTick = 0;
                    break;
                }
                case TcpManager::Event::Message:
                    messages.push_back(std::string("Peer: ") + event.data.message.message);
                    break;
                case TcpManager::Event::SDLEvent: {
                    if (pongWindowOpen && isMaster && event.fromPeer) {
                        SDL_Event& evt = event.data.sdlEvent.event;
                        switch(evt.type) {
                        case SDL_KEYDOWN:
                            if (evt.key.keysym.sym == SDLK_a) {
                                pong.topP.move(-0.5, 0.0f, pong.width);
                                pongUpdate = true;
                            } else if (evt.key.keysym.sym == SDLK_d) {
                                pong.topP.move(0.5, 0.0f, pong.width);
                                pongUpdate = true;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                }
                case TcpManager::Event::PongState: {
                    if (pongWindowOpen && !isMaster && event.fromPeer) {
                        pong = event.data.pongState.pong;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    void end_process() {
        Logger logger("Pong");
        if (pongWindowOpen && isMaster && pongUpdate) {
            manager.outgoingQueue.push(TcpManager::Event{
                .type = TcpManager::Event::PongState,
                .data = { .pongState = {
                    .pong = pong
                }}
            });
        }
    }

    void drawChat() {
        if (chatWindowOpen) {
            bool isOpen = true;
            if (ImGui::Begin("Game on", &isOpen)) {
                ImGui::Text("Connection Established");
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

    void drawPong(SDL_Renderer* renderer) {
        (void)renderer;
        if (!pongWindowOpen) return;

        bool isOpen = true;
        ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Pong", &isOpen)) {
            ImGui::Text("Role: %s", isMaster ? "Master" : "Client");
            ImGui::Text("Controls: A / D");
            ImGui::Separator();

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
        ImGui::End();

        if (!isOpen) {
            manager.disconnect();
        }
    }
};

#endif