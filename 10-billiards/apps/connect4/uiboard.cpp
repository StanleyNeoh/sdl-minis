#include "uiboard.hpp"
#include "engine.hpp"
#include <cstring>

namespace Apps {
namespace _Connect4 {

bool UiBoard::reqAddPiece(const Engine& engine, int col, Color turn) {
    EngineData req {
        .type = TypeMove,
        .data = { .move = {
            .col = col,
            .turn = turn,
        }}
    };
    engine.reqPipe.push(req);
    return true;
}

void UiBoard::addPiece(int col, Color color, int r) {
    if (r < 0) return;
    UiPiece& p = pieces[pieces_l++];
    p.color = color;
    p.r = r;
    p.c = col;
    p.fr = M;
    p.marked = false;
}

void UiBoard::setWinner(Color winner, BoardLoc* _marked) {
    if (winner == Color_None) return;
    for (int i = 0; i < 4; i++) {
        marked[_marked[i].r][_marked[i].c] = true;
    }
}

void UiBoard::clear() {
    pieces_l = 0;
    winner = Color_None;
    std::memset(&marked, 0, sizeof(marked));
}

}}

