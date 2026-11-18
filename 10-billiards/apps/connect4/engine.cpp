#include "engine.hpp"
#include "board.hpp"
#include <iostream>

#define BIG 1000000

namespace Apps {
namespace _Connect4 {

namespace {
    int countWinningSpots(const Board& board, Color turn, int req = 4) {
        static const std::array<std::pair<int, int>, 8> steps{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}}};
        auto& b = board.board;
        int count = 0;
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                if (b[r][c] != Color_None) continue;
                for (auto& p: steps) {
                    int dr = p.first;
                    int dc = p.second;
                    bool success = true;
                    int i = 1;
                    for (; i < req; i++) {
                        int nr = r + i * dr;
                        int nc = c + i * dc;
                        if (nr < 0 || nr >= M || nc < 0 || nc >= N || b[nr][nc] != turn) {
                            success = false;
                        };
                    }
                    // if (success) {
                    //     std::cout << key<< ": (" << r << " + "<< dr << ", " << c << " + " << dc << ")\n";
                    // }
                    count += success;
                }
            }
        }
        return count;
    }


    Score heuristic(const Board& board, Color turn) {
        Color other = switchTurn(turn);
        Score score{0, 0, 0};
        score.b += countWinningSpots(board, turn, 4);
        score.b -= countWinningSpots(board, other, 4);
        score.c += countWinningSpots(board, turn, 3);
        score.c -= countWinningSpots(board, other, 3);
        return score;
    }

    Score search_AB(Board& board, Color turn, Action* action = nullptr, int depth = 7, Score a = {-BIG, 0, 0}, Score b = {BIG, 0, 0}) {
        if (depth == 0) return heuristic(board, turn);
        Color other = switchTurn(turn);

        Score bestScore = {-BIG, -BIG, -BIG};
        int bestMove = -1;
        for (int c = 0; c < N; c++) {
            if (board.addPiece(c, turn) < 0) continue;
            Color winner = board.checkWin();
            Score score;
            if (winner == turn) {
                score = {depth, 0, 0};
            } else if (winner == other) {
                score = {-depth, 0, 0};
            } else {
                score = -search_AB(board, other, nullptr, depth-1, -b, -a);
            }
            board.remPiece(c);
            if (score > bestScore) {
                bestScore = score;
                bestMove = c;
            }
            a = std::max(a, bestScore);
            if (action != nullptr) {
                action->bestMove = bestMove;
                action->scores[c] = score; 
            }
            if (bestScore >= b) return bestScore;
        }
        if (bestMove == -1)  {
            return {0, 0, 0};
        }
        return bestScore;
    }
}

Action search_move(Board board, Color turn, int depth) {
    Action action;
    search_AB(board, turn, &action, depth);
    return action;
}


}}