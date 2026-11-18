#include "uiboard.hpp"
#include <cstring>

namespace Apps {
namespace _Connect4 {

int UiBoard::addPiece(int col, Color color) {
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

void UiBoard::clear() {
    pieces_l = 0;
    std::memset(&board, 0, sizeof(board));
    std::memset(&marked, 0, sizeof(marked));
}

}}

