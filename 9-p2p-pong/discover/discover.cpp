#include "discover.hpp"
#include "app/app.hpp"
#include "p2p/tcp_manager.hpp"

namespace Discover {
    Discover discover;

    void Discover::draw() {
        ImGui::Begin("LAN Users");
        ImGui::Text("User: %s", config.ownLoc.name);
        if (ImGui::BeginTable("neighbour_table", 3, ImGuiTableFlags_Borders, ImVec2(-FLT_MIN, 0.0))) {
            ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Connect", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableHeadersRow();
            {
                for (auto& p: neighbours) {
                    Loc& neigh = p.second;
                    std::string address;
                    std::string state;
                    bool isHost = neigh.address == config.ownLoc.address;
                    address = to_string(neigh);
                    state = to_string(isHost ? Loc::IsHost : neigh.state);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", address.data());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", state.data());
                    ImGui::TableSetColumnIndex(2);
                    float cellWidth = ImGui::GetContentRegionAvail().x;
                    ImGui::PushID(address.data());
                    ImGui::BeginDisabled(neigh.state != Loc::Available);
                    if (ImGui::Button("Chat", ImVec2{cellWidth, 20.0f})) {
                        if (isHost) {
                            App::app.role_type = P2P::RoleType_SinglePlayer;
                            App::app.reset_to_state(App::AppState_GameSelect);
                        } else {
                            P2P::tcp_manager.connect(neigh.address);
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
}