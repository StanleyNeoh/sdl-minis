#ifndef APP_HPP
#define APP_HPP

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <string>
#include "discover.hpp"
#include "p2pTcp.hpp"
#include "utils.hpp"
#include "imgui.h"
struct Pong {
    enum State {
        State_Ongoing,
        State_Top_Wins,
        State_Bot_Wins,
    };

    struct Paddle {
        Vec2 pos;
        float w;

        Paddle(float x, float y, float w): pos(x, y), w(w) {}

        void move(float dx) {
            pos.x += dx;
        }
    };

    struct Ball {
        Vec2 pos;
        Vec2 vel;
        float r;
        Ball(float x, float y, float r = 1.0): pos(x, y), vel(Vec2::rand_unit()), r(r) {}

        struct OverlapInfo {
            Vec2 normal;
        };

        bool overlap_pt(float x, float y, OverlapInfo& info) const {
            float dx = pos.x - x;
            float dy = pos.y - y;
            float d2 = dx * dx + dy * dy;
            if (r * r > d2) return false;
            info.normal = Vec2(dx, dy);
            info.normal.normalise();
            return true;
        }

        bool overlap_hsec(float x, float y, float l, OverlapInfo& info) const {
            if (pos.x < x || pos.x > x + l || abs(pos.y - y) > r) return false;
            info.normal = pos.y > y ? Vec2(0, 1) : Vec2(0, -1);
            return true;
        }

        bool overlap_paddle(const Paddle& pad, OverlapInfo& info) const {
            return (
                overlap_pt(pad.pos.x, pad.pos.y, info) 
                || overlap_pt(pad.pos.x + pad.w, pad.pos.y, info)
                || overlap_hsec(pad.pos.x, pad.pos.y, pad.w, info)
            );
        }
    };

    float width;
    float height;
    Ball ball;
    Paddle topP;
    Paddle botP;

    Pong(float width = 30.0, float height = 30.0, float ball_r = 1.0, float pad_w = 3.0, float pad_m = 1.0): 
        width(width), 
        height(height),
        ball(width / 2, height / 2),
        topP(width - pad_w / 2, pad_m, pad_w),
        botP(width - pad_w / 2, height - pad_m, pad_w)
    {}

    State step(float dt) {
        ball.pos.x = ball.pos.x + ball.vel.x * dt;
        ball.pos.y = ball.pos.y + ball.vel.y * dt;
        if (ball.pos.x < 0) {
            ball.pos.x = 0;
            if (ball.vel.x < 0) ball.vel.x = -ball.vel.x;
        }
        if (ball.pos.y < 0) {
            return State_Bot_Wins;
        }
        if (ball.pos.x > width) {
            ball.pos.x = width;
            if (ball.vel.x > 0) ball.vel.x = -ball.vel.x;
        }
        if (ball.pos.y > height) {
            return State_Top_Wins;
        }
        Ball::OverlapInfo info;
        if (ball.overlap_paddle(topP, info)) {
            float scale = ball.vel.x * info.normal.x + ball.vel.y * info.normal.y;
            ball.vel.x -= info.normal.x * 2 * scale;
            ball.vel.y -= info.normal.y * 2 * scale;
        }
        if (ball.overlap_paddle(botP, info)) {
            float scale = ball.vel.x * info.normal.x + ball.vel.y * info.normal.y;
            ball.vel.x -= info.normal.x * 2 * scale;
            ball.vel.y -= info.normal.y * 2 * scale;
        }
        return State_Ongoing;
    }
};

struct App {
    Discover discover;
    TcpManager manager;

    // Chat State
    bool chatWindowOpen = false;
    std::vector<std::string> messages;
    char chatInput[128] = {0};

    App(std::string_view name, u_int16_t gamePort):
        discover(Discover::Config(name, gamePort)),
        manager(TcpManager::Config(gamePort)) {}
    
    void process_events() {
        Logger logger("App");
        discover.process_events();
        TcpManager::Event event;
        while (manager.incomingQueue.try_pop(event)) {
            switch (event.type) {
                case TcpManager::Event::ConnectEvent: {
                    if (event.data.connectEvent.success) {
                        logger.log("Connected to ", event.data.connectEvent.addr);
                        chatWindowOpen = true;
                    } else {
                        chatWindowOpen = false;
                    }
                    break;
                }
                case TcpManager::Event::DisconnectEvent: {
                    if (!event.data.disconnectEvent.success) break;
                    logger.log("Disconnected from ", event.data.disconnectEvent.addr);
                    messages.clear();
                    chatWindowOpen = false;
                    break;
                }
                case TcpManager::Event::Message:
                    messages.push_back(std::string("Peer: ") + event.data.message.message);
                    break;
                default:
                    break;
            }
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