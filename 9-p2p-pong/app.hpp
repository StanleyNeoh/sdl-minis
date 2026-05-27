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
    Pong pong;

    App(std::string_view name, u_int16_t gamePort):
        discover(Discover::Config(name, gamePort)),
        manager(TcpManager::Config(gamePort)) {}
    
    void begin_process() {
        pongUpdate = false;
    }
    
    void process_sdl_events(const SDL_Event& event) {
        if (!pongWindowOpen) return;
        switch (event.type) {
            case SDL_KEYDOWN: {
                auto key = event.key.keysym.sym;
                if (isMaster) {
                    switch (key) {
                        case SDLK_a:
                            pong.botP.move(-0.5);
                            pongUpdate = true;
                            break;
                        case SDLK_d:
                            pong.botP.move(0.5);
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
                                pong.topP.move(-0.5);
                                pongUpdate = true;
                            } else if (evt.key.keysym.sym == SDLK_d) {
                                pong.topP.move(0.5);
                                pongUpdate = true;
                            }
                        }
                    }
                }
                case TcpManager::Event::PongState: {
                    if (pongWindowOpen && !isMaster && event.fromPeer) {
                        pong = event.data.pongState.pong;
                    }
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
        if (app.pongWindowOpen) {
            logger.log(app.pong);
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
};

#endif