#ifndef APPS_CONNECT4_HPP
#define APPS_CONNECT4_HPP

#include "../../textures/connect4/board_frame.hpp"
#include "imgui.h"

#include "uiboard.hpp"
#include "engine.hpp"
#include "../../utils/pipe.hpp"

namespace Apps {
namespace _Connect4 {
    struct App {
        using BoardFrame = Textures::Connect4::BoardFrame;
        static constexpr const char* name = "Connect 4";

        UiBoard board;
        Engine engine;
        Color turn = PlayerColor;

        int colSelected(float x, float originX, float regionW);
        void handleEngineData(EngineData& res);
        void drawControls();
        void drawPieces(ImDrawList* drawList, Uint64 dt, ImVec2 origin, ImVec2 region);
        void drawBoard(ImDrawList* drawList, ImVec2 origin, ImVec2 region);
        void draw(Uint64 dt);
        void init();
        void deinit();
    };
}

using Connect4 = _Connect4::App;

}

#endif
