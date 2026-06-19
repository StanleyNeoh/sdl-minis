#ifndef CHAT_CHAT_HPP
#define CHAT_CHAT_HPP

#include <vector>
#include <string>
#include "imgui.h"
#include "p2p/p2p.hpp"
#include "lib/metap/metap.hpp"

namespace Chat {
    struct Chat {
        std::vector<std::string> messages;
        char chatInput[128] = {0};

        void reset() {
            chatInput[0] = '\0';
            messages.clear();
        }

        void process_setup() {};
        void process_sdl_event(const SDL_Event& event) {};
        void process_packet(P2P::Packet& packet) {
            static MetaP::Callbacks callbacks(
                [&](const P2P::MessageBody& message) {
                    messages.push_back(std::string("Peer: ") + message.message);
                }
            );
            callbacks.dispatch<
                MetaP::TT_TVIsEquals<P2P::TV_BodyType>::type,
                MetaP::TO_VariantCast
            >(packet.type, packet.body);
        };
        void process_takedown() {};

        void draw() {
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

        }
    };

    extern Chat chat;
}

#endif