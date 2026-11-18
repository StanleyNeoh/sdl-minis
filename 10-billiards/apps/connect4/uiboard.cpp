#include "uiboard.hpp"
#include "engine.hpp"
#include <cstring>

namespace Apps {
namespace _Connect4 {

void UiBoard::addPiece(const Engine& engine, int col, Color turn) {
    if (lockUi) return;
    EngineData req {
        .type = TypeMove,
        .data = { .move = {
            .col = col,
            .turn = turn,
        }}
    };
    engine.reqPipe.push(req);
    lockUi = true;
}


void UiBoard::handleEngineData(const EngineData& res) {
    switch (res.type) {
    case TypeMove: {
        int r = res.data.move.res.r;
        lockUi = false;
        if (r < 0) return;
        UiPiece& p = pieces[pieces_l++];
        p.color = res.data.move.turn;
        p.r = r;
        p.c = res.data.move.col;
        p.fr = M;
        p.marked = false;
        return;
    }
    case TypeWin: {
        winner = res.data.win.res.color;
        if (winner != Color_None) {
            auto _marked = res.data.win.res.marked;
            for (int i = 0; i < 4; i++) {
                marked[_marked[i].r][_marked[i].c] = true;
            }
            lockUi = true;
        }
        return;
    }
    }
}

void UiBoard::clear() {
    lockUi = false;
    pieces_l = 0;
    winner = Color_None;
    std::memset(&marked, 0, sizeof(marked));
}

}}

