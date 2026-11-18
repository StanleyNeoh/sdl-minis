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
    int pieces_l = 0;
    std::array<UiPiece, M * N> pieces;
    Color winner = Color_None;
    bool marked[M][N] {};

    bool reqAddPiece(const Engine& engine, int col, Color color);
    void addPiece(int col, Color color, int r);
    void setWinner(Color winner, BoardLoc* _marked);
    void clear();
};

}}

#endif