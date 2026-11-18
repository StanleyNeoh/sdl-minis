#ifndef APPS_CONNECT4_BOARD_HPP
#define APPS_CONNECT4_BOARD_HPP

#include "../../textures/connect4/board_frame.hpp"
#include <array>

namespace Apps {
namespace _Connect4 {
    using BoardFrame = Textures::Connect4::BoardFrame;
    enum Color {
        Color_None = 0,
        Color_Red = 1,
        Color_Blue = 2,
    };
    static constexpr Color BotColor = Color_Blue;
    static constexpr Color PlayerColor = Color_Red;
    static constexpr int M = BoardFrame::M;
    static constexpr int N = BoardFrame::N;

    Color switchTurn(Color turn);

    struct Board {
        Color board[M][N] {};
        int next[N] {};

        int addPiece(int col, Color color);
        int remPiece(int col);

        Color checkWin(bool** marked = nullptr) const;
    };
}}

#endif