#include "board.hpp"

#include <cstring>

namespace Apps {
namespace _Connect4 {

Color switchTurn(Color turn) {
    switch (turn) {
        case BotColor:
            return PlayerColor;
        case PlayerColor:
            return BotColor;
    }
    return Color_None;
}

int Board::addPiece(int col, Color color) {
    if (col < 0 || col >= N || color == Color_None) return -1;
    int r = next[col];
    if (r >= M) return -1;
    board[r][col] = color;
    next[col]++;
    return r;
}

int Board::remPiece(int col) {
    if (col < 0 || col >= N) return -1;
    if (next[col] == 0) return -1;
    int r = --next[col];
    board[r][col] = Color_None;
    return r;
}

Color Board::checkWin(bool marked[M][N]) const {
    auto check_loop = [](Color p, Color& pp, int& pc) {
        if (p == pp) {
            pc++;
        } else {
            pp = p;
            pc = 1;
        }
        if (pc >= 4 && pp != Color_None) {
            return true;
        }
        return false;
    };

    // Check row
    for (int r = 0; r < M; r++) {
        Color pp = Color_None;
        int pc = 0;
        for (int c = 0; c < N; c++) {
            if (check_loop(board[r][c], pp, pc)) {
                if (marked != nullptr) {
                    for (int _c = c - 3; _c <= c; _c++) {
                        marked[r][_c] = true;
                    }
                }
                return pp;
            }
        }
    }

    // Check col
    for (int c = 0; c < N; c++) {
        Color pp = Color_None;
        int pc = 0;
        for (int r = 0; r < M; r++) {
            if (check_loop(board[r][c], pp, pc)) {
                if (marked != nullptr) {
                    for (int _r = r - 3; _r <= r; _r++) {
                        marked[_r][c] = true;
                    }
                }
                return pp;
            }
        }
    }

    // Check tl-br
    // 0 <= c <= N-1, 0 <= r <= M-1, -N+1 <= i <= M-1
    // max(0, i) <= r = c + i <= min(N-1+i, M-1)
    for (int i = -N+1; i <= M-1; i++) {
        Color pp = Color_None;
        int pc = 0;
        for (int r = std::max(0, i); r <= std::min(N-1+i, M-1); r++) {
            int c = r - i;
            if (check_loop(board[r][c], pp, pc)) {
                if (marked != nullptr) {
                    for (int _r = r - 3; _r <= r; _r++) {
                        marked[_r][_r - i] = true;
                    }
                }
                return pp;
            }
        }
    }

    // Check tr-bl
    // 0 <= c <= N-1, 0 <= r <= M-1, 0 <= i <= M+N-1
    // 0 <= r = i - c <= i
    for (int i = 0; i <= M+N-1; i++) {
        Color pp = Color_None;
        int pc = 0;
        for (int r = std::max(0, i - N + 1); r <= std::min(i, M-1); r++) {
            int c = i - r;
            if (check_loop(board[r][c], pp, pc)) {
                if (marked != nullptr) {
                    for (int _r = r - 3; _r <= r; _r++) {
                        marked[_r][i - _r] = true;
                    }
                }
                return pp;
            }
        }
    }
    return Color_None;
}

void Board::clear() {
    std::memset(board, 0, sizeof(board));
    std::memset(next, 0, sizeof(next));
}

}}