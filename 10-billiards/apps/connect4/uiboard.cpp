#include "uiboard.hpp"
#include <cstring>

namespace Apps {
namespace _Connect4 {

int UiBoard::addPiece(int col, Color color) {
    if (winner != Color_None) return -1;
    int r = board.addPiece(col, color);
    if (r < 0) return r;
    UiPiece& p = pieces[pieces_l++];
    p.color = color;
    p.r = r;
    p.c = col;
    p.fr = M;
    p.marked = false;
    return r;
}

bool UiBoard::checkWin() {
    winner = board.checkWin(marked);
    return winner != Color_None;
}


void UiBoard::clear() {
    board.clear();
    pieces_l = 0;
    winner = Color_None;
    std::memset(&marked, 0, sizeof(marked));
}

}}

