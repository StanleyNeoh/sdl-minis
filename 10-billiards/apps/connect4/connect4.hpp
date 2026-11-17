#ifndef APPS_CONNECT4_HPP
#define APPS_CONNECT4_HPP

#include "../../textures/connect4/board_frame.hpp"
#include <SDL.h>
#include "imgui.h"

namespace Apps {

struct Connect4 {
    static constexpr const char* name = "Connect 4";

    void draw() {
        SDL_Texture* board_tex = Textures::Connect4::BoardFrame::get();
        ImGui::Image((ImTextureID) board_tex, ImVec2(1000.0f, 1000.0f));
    }

    void init() {}
    void deinit() {}
};

}

#endif
