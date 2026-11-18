#ifndef APPS_CONNECT4_UIBOARD_HPP
#define APPS_CONNECT4_UIBOARD_HPP

#include "board.hpp"
#include "engine.hpp"

namespace Apps {
namespace _Connect4 {

struct UiPiece {
    Color color = Color_None;
    int r = -1;
    int c = -1;
    float fr = -1;
    bool marked = false;
};

struct UiBoard {
    bool lockUi = false;
    int pieces_l = 0;
    std::array<UiPiece, M * N> pieces;
    Color winner = Color_None;
    bool marked[M][N] {};

    void addPiece(const Engine& engine, int col, Color color);
    void handleEngineData(const EngineData& res);
    void clear();
};

}}

#endif