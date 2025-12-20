
#ifndef LIB_ENGINE_HPP
#define LIB_ENGINE_HPP

#include "utils.hpp"
#include <iostream>
#include <array>
#include <climits>
#include <atomic>
#include <vector>

#define BIG 1000000

using CellKey = char;
constexpr static CellKey NoneKey = '.';
constexpr static CellKey BotKey = 'B';
constexpr static CellKey PlayerKey = 'R';
static CellKey other_player(CellKey key) {
    switch(key) {
    case BotKey:
        return PlayerKey;
    case PlayerKey:
        return BotKey;
    default:
        return NoneKey;
    }
}

struct GridLoc { 
    int r;
    int c;
    bool operator==(const GridLoc& other) const {
        return r == other.r && c == other.c;
    }
};

struct Score {
    int a;
    int b;
    int c;

    bool operator==(const Score& other) const {
        return a == other.a && b == other.b && c == other.c;
    }

    bool operator<(const Score& other) const {
        if (a != other.a) return a < other.a;
        if (b != other.b) return b < other.b;
        return c < other.c;
    }

    bool operator>(const Score& other) const {
        return other < *this;
    }

    bool operator<=(const Score& other) const {
        return *this == other || *this < other;
    }

    bool operator>=(const Score& other) const {
        return other <= *this;
    }

    Score operator-() const {
        return Score{-a, -b, -c};
    }

    friend std::ostream& operator<<(std::ostream& o, const Score& score) {
        o << score.a << "," << score.b << "," << score.c;
        return o;
    }
};


template<>
struct std::hash<GridLoc> {
    std::size_t operator()(const GridLoc& f) const {
        return std::hash<int>{}(f.r) ^ std::hash<int>{}(f.r);
    }
};

template <int M, int N>
struct Grid {
    std::array<std::array<CellKey, N>, M> grid;

    struct Action {
        int bestMove;
        std::array<Score, N> scores;
    };


    Grid() {
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                grid[r][c] = NoneKey;
            }
        }
    }

    int drop_piece(int col_i, CellKey turn) {
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

    CellKey check_win(std::vector<GridLoc>* marked = nullptr, int req = 4) {
        auto check_loop = [](int req, CellKey p, CellKey& pp, int& pc) {
            if (p == pp) {
                pc++;
            } else {
                pp = p;
                pc = 1;
            }
            if (pc >= req && pp != NoneKey) {
                return true;
            }
            return false;
        };

        // Check row
        for (int r = 0; r < M; r++) {
            CellKey pp = NoneKey;
            int pc = 0;
            for (int c = 0; c < N; c++) {
                if (check_loop(req, grid[r][c], pp, pc)) {
                    if (marked != nullptr) {
                        for (int _c = c - req + 1; _c <= c; _c++) {
                            marked->push_back({r, _c});
                        }
                    }
                    return pp;
                }
            }
        }

        // Check col
        for (int c = 0; c < N; c++) {
            CellKey pp = NoneKey;
            int pc = 0;
            for (int r = 0; r < M; r++) {
                if (check_loop(req, grid[r][c], pp, pc)) {
                    if (marked != nullptr) {
                        for (int _r = r - req + 1; _r <= r; _r++) {
                            marked->push_back({_r, c});
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
            CellKey pp = NoneKey;
            int pc = 0;
            for (int r = std::max(0, i); r <= std::min(N-1+i, M-1); r++) {
                int c = r - i;
                if (check_loop(req, grid[r][c], pp, pc)) {
                    if (marked != nullptr) {
                        for (int _r = r - req + 1; _r <= r; _r++) {
                            marked->push_back({_r, _r - i});
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
            CellKey pp = NoneKey;
            int pc = 0;
            for (int r = std::max(0, i - N + 1); r <= std::min(i, M-1); r++) {
                int c = i - r;
                if (check_loop(req, grid[r][c], pp, pc)) {
                    if (marked != nullptr) {
                        for (int _r = r - req + 1; _r <= r; _r++) {
                            marked->push_back({_r, i - _r});
                        }
                    }
                    return pp;
                }
            }
        }
        return NoneKey;
    }

    int count_winning_spots(CellKey key, int req = 4) {
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

    Action search(CellKey turn, int depth = 7) {
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

    Action bot_plays(CellKey turn) {
        auto action = search(turn);
        return action;
    }

    Score heuristic(CellKey turn) {
        CellKey other = other_player(turn);
        Score score{0, 0, 0};
        score.b += count_winning_spots(turn, 4);
        score.b -= count_winning_spots(other, 4);
        score.c += count_winning_spots(turn, 3);
        score.c -= count_winning_spots(other, 3);
        return score;
    }

    Score search_AB(CellKey turn, Action* action = nullptr, int depth = 7, Score a = {-BIG, 0, 0}, Score b = {BIG, 0, 0}) {
        if (depth == 0) return heuristic(turn);
        CellKey other = other_player(turn);
        Score bestScore = {-BIG, -BIG, -BIG};
        int bestMove = -1;
        for (int c = 0; c < N; c++) {
            if (drop_piece(c, turn) < 0) continue;
            CellKey winner = check_win();
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
    std::atomic<CellKey> turn = PlayerKey;
    std::atomic<CellKey> winner = NoneKey;
    std::atomic<int> botmove_r = -1;
    std::atomic<int> botmove_c = -1;
    std::vector<GridLoc> marked;


    bool player_plays(int col, int& r) {
        if (turn.load(std::memory_order_acquire) != PlayerKey) return false;
        r = grid.drop_piece(col, PlayerKey);
        if (r < 0) return false;
        std::cout << "Player plays: " << col << "\n";
        std::cout << grid << "\n";
        handover_turn();
        return r >= 0;
    }

    void bot_loop() {
        std::cout << "Starting bot loop" << std::endl;
        while (winner.load() == NoneKey) {
            if (turn.load(std::memory_order_acquire) == BotKey) {
                auto action = grid.bot_plays(BotKey);
                int c = action.bestMove;
                int r = grid.drop_piece(c, BotKey);
                std::cout << "Bot Plays: " << c << "\n";
                for (auto s: action.scores) {
                    std::cout << s << "|";
                }
                std::cout << "\n" << grid << "\n";
                botmove_r.store(r);
                botmove_c.store(c);
                handover_turn();
            }
            SDL_Delay(100);
        }
        std::cout << "Ending bot loop" << std::endl;
    }
 
    void handover_turn() {
        winner.store(grid.check_win(&marked));
        if (winner.load() == NoneKey) {
            turn.store(other_player(turn), std::memory_order_release);
        } else {
            turn.store(NoneKey, std::memory_order_release);
        }
    }

    bool query_botmove(
        int& _botmove_r,
        int& _botmove_c
    ) {
        _botmove_r = botmove_r.load();
        _botmove_c = botmove_c.load();
        if (_botmove_r < 0) return false;
        botmove_r.store(-1);
        botmove_c.store(-1);
        return true;
    }

    CellKey query_win(std::vector<GridLoc>& _marked) {
        _marked = marked;
        return winner.load();
    }
};

#endif