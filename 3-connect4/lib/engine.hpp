
#ifndef LIB_ENGINE_HPP
#define LIB_ENGINE_HPP

#include "utils.hpp"
#include "network.hpp"
#include <iostream>
#include <array>
#include <climits>
#include <atomic>
#include <vector>

#define BIG 1000000


template <int M, int N, int REQ = 4>
struct Grid {
    std::array<std::array<Cell, N>, M> grid;

    struct Action {
        int bestMove;
        std::array<Score, N> scores;
    };

    Grid() {
        for (auto& row: grid) {
            row.fill(NoneKey);
        }
    }

    int drop_piece(int col_i, Cell turn) {
        for (int r = M-1; r >= 0; r--) {
            if (grid[r][col_i] != NoneKey) continue;
            grid[r][col_i] = turn;
            return r;
        }
        return -1;
    }

    int undrop_piece(int col_i, int r = -1) {
        if (r >= 0) {
            grid[r][col_i] = NoneKey;
            return r;
        }
        for (int r = 0; r < M; r++) {
            if (grid[r][col_i] == NoneKey) continue;
            grid[r][col_i] = NoneKey;
            return r;
        }
        return -1;
    }

    Cell check_win(std::array<GridLoc, REQ>* marked = nullptr) {
        auto check_loop = [](Cell p, Cell& pp, int& pc) {
            if (p == pp) {
                pc++;
            } else {
                pp = p;
                pc = 1;
            }
            if (pc >= REQ && pp != NoneKey) {
                return true;
            }
            return false;
        };

        // Check row
        for (int r = 0; r < M; r++) {
            Cell pp = NoneKey;
            int pc = 0;
            for (int c = 0; c < N; c++) {
                if (check_loop(grid[r][c], pp, pc)) {
                    if (marked != nullptr) {
                        for (int _i = 0, _c = c - REQ + 1; _c <= c; _c++) {
                            (*marked)[_i++] = {r, _c};
                        }
                    }
                    return pp;
                }
            }
        }

        // Check col
        for (int c = 0; c < N; c++) {
            Cell pp = NoneKey;
            int pc = 0;
            for (int r = 0; r < M; r++) {
                if (check_loop(grid[r][c], pp, pc)) {
                    if (marked != nullptr) {
                        for (int _i = 0, _r = r - REQ + 1; _r <= r; _r++) {
                            (*marked)[_i++] = {_r, c};
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
            Cell pp = NoneKey;
            int pc = 0;
            for (int r = std::max(0, i); r <= std::min(N-1+i, M-1); r++) {
                int c = r - i;
                if (check_loop(grid[r][c], pp, pc)) {
                    if (marked != nullptr) {
                        for (int _i = 0, _r = r - REQ + 1; _r <= r; _r++) {
                            (*marked)[_i++] = {_r, _r - i};
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
            Cell pp = NoneKey;
            int pc = 0;
            for (int r = std::max(0, i - N + 1); r <= std::min(i, M-1); r++) {
                int c = i - r;
                if (check_loop(grid[r][c], pp, pc)) {
                    if (marked != nullptr) {
                        for (int _i = 0, _r = r - REQ + 1; _r <= r; _r++) {
                            (*marked)[_i++] = {_r, i - _r};
                        }
                    }
                    return pp;
                }
            }
        }
        return NoneKey;
    }

    int count_winning_spots(Cell key, int req = 4) {
        static const std::array<std::pair<int, int>, 8> steps{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}}};
        int count = 0;
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                if (grid[r][c] != NoneKey) continue;
                for (auto& p: steps) {
                    int dr = p.first;
                    int dc = p.second;
                    bool success = true;
                    int i = 1;
                    for (; i < req; i++) {
                        int nr = r + i * dr;
                        int nc = c + i * dc;
                        if (nr < 0 || nr >= M || nc < 0 || nc >= N || grid[nr][nc] != key) {
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

    Action search(Cell turn, int depth = 7) {
        Grid grid(*this);
        Action action;
        grid.search_AB(turn, &action, depth);
        return action;
    }

    template <int _M, int _N>
    friend std::ostream& operator<<(std::ostream& o, const Grid<_M, _N>& g) {
        for (int i = 0; i < _M; i++) {
            for (int j = 0; j < _N; j++) {
                o << g.grid[i][j] << " ";
            }
            o << "\n";
        }
        return o;
    }

    Action bot_plays(Cell turn) {
        auto action = search(turn);
        return action;
    }

    Score heuristic(Cell turn) {
        Cell other = other_player(turn);
        Score score{0, 0, 0};
        score.b += count_winning_spots(turn, 4);
        score.b -= count_winning_spots(other, 4);
        score.c += count_winning_spots(turn, 3);
        score.c -= count_winning_spots(other, 3);
        return score;
    }

    Score search_AB(Cell turn, Action* action = nullptr, int depth = 7, Score a = {-BIG, 0, 0}, Score b = {BIG, 0, 0}) {
        if (depth == 0) return heuristic(turn);
        Cell other = other_player(turn);
        Score bestScore = {-BIG, -BIG, -BIG};
        int bestMove = -1;
        for (int c = 0; c < N; c++) {
            if (drop_piece(c, turn) < 0) continue;
            Cell winner = check_win();
            Score score;
            if (winner == turn) {
                score = {depth, 0, 0};
            } else if (winner == other) {
                score = {-depth, 0, 0};
            } else {
                score = -search_AB(other, nullptr, depth-1, -b, -a);
            }
            undrop_piece(c);
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

};

struct Engine {
    Grid<6, 7> grid;
    Network& net;
    
    Cell winner = NoneKey;
    std::array<GridLoc, 4> marked;

    Engine(Network& net): net(net) {}

    void handle_player_move(const Move& move) {
        if (move.key != PlayerKey) return;
        if (player_plays(move.c)) return;
        if (bot_plays()) return;
    }

    bool player_plays(int c) {
        int r = grid.drop_piece(c, PlayerKey);
        BotEvent event{};
        event.move = {NetworkEventType::MOVE, PlayerKey, r, c};
        net.bot_events.block_push(event);
        if (r < 0) {
            std::cout << "Player cannot play " << c << "\n";
        } else {
            std::cout << "Player plays: " << c << "\n";
            std::cout << grid << "\n";
        };
        return handover_turn();
    }

    bool bot_plays() {
        auto action = grid.bot_plays(BotKey);
        int c = action.bestMove;
        int r = grid.drop_piece(c, BotKey);

        std::cout << "Bot Plays: " << c << "\n";
        for (auto s: action.scores) {
            std::cout << s << "|";
        }
        std::cout << "\n" << grid << "\n";
        BotEvent event{};
        event.move = {NetworkEventType::MOVE, BotKey, r, c};
        net.bot_events.block_push(event);
        return handover_turn();
    }

    void engine_loop() {
        std::cout << "Starting bot loop" << std::endl;
        UiEvent ui_event;
        while (winner == NoneKey) {
            while (net.ui_events.pop(ui_event)) {
                switch(ui_event.type) {
                case NetworkEventType::MOVE:
                    handle_player_move(ui_event.move);
                    break;
                default:
                    break;
                }
            }
            SDL_Delay(100);
        }
        std::cout << "Ending bot loop" << std::endl;
    }
 
    bool handover_turn() {
        Cell winner = grid.check_win(&marked);
        switch(winner) {
        case PlayerKey:
            std::cout << "Player has won\n";
            break;
        case BotKey:
            std::cout << "Bot has won\n";
            break;
        default:
            return false;
        }
        BotEvent event{};
        event.game_end = {NetworkEventType::GAME_END, winner, marked};
        net.bot_events.block_push(event);
        return true;
    }
};

#endif