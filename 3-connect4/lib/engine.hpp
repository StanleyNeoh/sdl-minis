
#ifndef LIB_ENGINE_HPP
#define LIB_ENGINE_HPP

#include "utils.hpp"
#include <iostream>

#define BIG 1000000

using CellKey = char;
constexpr static CellKey PlayerNoneKey = '.';
constexpr static CellKey PlayerBlueKey = 'B';
constexpr static CellKey PlayerRedKey = 'R';

struct Cell {
    char key;
    bool marked;

    Cell(): Cell(PlayerNoneKey) {}
    Cell(char c): key(c), marked(false) {}

    static CellKey other_player(CellKey key) {
        switch(key) {
        case PlayerBlueKey:
            return PlayerRedKey;
        case PlayerRedKey:
            return PlayerBlueKey;
        default:
            return PlayerNoneKey;
        }
    }

    void set(CellKey k) {
        key = k;
    }

    friend std::ostream& operator<<(std::ostream& o, const Cell& p) {
        if (p.marked) {
            o << '[' << p.key << ']';
        } else {
            o << ' ' << p.key << ' ';
        }
        return o;
    }

    bool operator==(const CellKey& k) const {
        return key == k;
    }

    bool operator!=(const CellKey& k) const {
        return key != k;
    }

    void mark() {
        marked = true;
    }

    void unmark() {
        marked = false;
    }

    bool is_marked() {
        return marked;
    }
};

template <int M, int N>
struct Grid {
    std::array<std::array<Cell, N>, M> grid;

    struct Action {
        int bestMove;
        std::array<int, N> scores = {-1};
    };


    Grid() {
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                grid[r][c] = Cell();
            }
        }
    }

    int drop_piece(int col_i, CellKey turn) {
        for (int r = M-1; r >= 0; r--) {
            if (grid[r][col_i] != PlayerNoneKey) continue;
            grid[r][col_i].set(turn);
            return r;
        }
        return -1;
    }

    int undrop_piece(int col_i, int r = -1) {
        if (r >= 0) {
            grid[r][col_i].set(PlayerNoneKey);
            return r;
        }
        for (int r = 0; r < M; r++) {
            if (grid[r][col_i] == PlayerNoneKey) continue;
            grid[r][col_i].set(PlayerNoneKey);
            return r;
        }
        return -1;
    }

    CellKey check_win(int req = 4) {
        auto check_loop = [](int req, Cell p, CellKey& pp, int& pc) {
            if (p.key == pp) {
                pc++;
            } else {
                pp = p.key;
                pc = 1;
            }
            if (pc >= req && pp != PlayerNoneKey) {
                return true;
            }
            return false;
        };

        // Check row
        for (int r = 0; r < M; r++) {
            CellKey pp = PlayerNoneKey;
            int pc = 0;
            for (int c = 0; c < N; c++) {
                if (check_loop(req, grid[r][c], pp, pc)) {
                    for (int _c = c - req + 1; _c <= c; _c++) {
                        grid[r][_c].mark();
                    }
                    return pp;
                }
            }
        }

        // Check col
        for (int c = 0; c < N; c++) {
            CellKey pp = PlayerNoneKey;
            int pc = 0;
            for (int r = 0; r < M; r++) {
                if (check_loop(req, grid[r][c], pp, pc)) {
                    for (int _r = r - req + 1; _r <= r; _r++) {
                        grid[_r][c].mark();
                    }
                    return pp;
                }
            }
        }

        // Check tl-br
        // 0 <= c <= N-1, 0 <= r <= M-1, -N+1 <= i <= M-1
        // max(0, i) <= r = c + i <= min(N-1+i, M-1)
        for (int i = -N+1; i <= M-1; i++) {
            CellKey pp = PlayerNoneKey;
            int pc = 0;
            for (int r = std::max(0, i); r <= std::min(N-1+i, M-1); r++) {
                int c = r - i;
                if (check_loop(req, grid[r][c], pp, pc)) {
                    for (int _r = r - req + 1; _r <= r; _r++) {
                        grid[_r][_r - i].mark();
                    }
                    return pp;
                }
            }
        }

        // Check tr-bl
        // 0 <= c <= N-1, 0 <= r <= M-1, 0 <= i <= M+N-1
        // 0 <= r = i - c <= i
        for (int i = 0; i <= M+N-1; i++) {
            CellKey pp = PlayerNoneKey;
            int pc = 0;
            for (int r = std::max(0, i - N + 1); r <= std::min(i, M-1); r++) {
                int c = i - r;
                if (check_loop(req, grid[r][c], pp, pc)) {
                    for (int _r = r - req + 1; _r <= r; _r++) {
                        grid[_r][i - _r].mark();
                    }
                    return pp;
                }
            }
        }
        return PlayerNoneKey;
    }

    Cell& get(int r, int c) {
        return grid[r][c];
    }

    int count_winning_spots(CellKey key, int req = 4) {
        static const std::array<std::pair<int, int>, 8> steps{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}}};
        int count = 0;
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                if (grid[r][c].key != PlayerNoneKey) continue;
                for (auto& p: steps) {
                    int dr = p.first;
                    int dc = p.second;
                    bool success = true;
                    int i = 1;
                    for (; i < req; i++) {
                        int nr = r + i * dr;
                        int nc = c + i * dc;
                        if (nr < 0 || nr >= M || nc < 0 || nc >= N || grid[nr][nc].key != key) {
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

    int bot_plays(CellKey turn) {
        auto action = search(turn);
        std::cout << "Bot: " << action.bestMove << "\n";
        for (int score: action.scores) {
            std::cout << score << ", ";
        }
        std::cout << "\n" << *this << "\n";
        return action.bestMove;
    }

private:
    int heuristic(CellKey turn) {
        CellKey other = Cell::other_player(turn);
        int score = 0;
        score += count_winning_spots(turn, 4) * 10;
        score -= count_winning_spots(other, 4) * 10;
        score += count_winning_spots(turn, 3);
        score -= count_winning_spots(other, 3);
        return score;
    }

    int search_AB(CellKey turn, Action* action = nullptr, int depth = 7, int a = -BIG, int b = BIG) {
        if (depth == 0) return heuristic(turn);
        CellKey other = Cell::other_player(turn);
        int bestScore = INT_MIN;
        int bestMove = -1;
        for (int c = 0; c < N; c++) {
            if (drop_piece(c, turn) < 0) continue;
            CellKey winner = check_win();
            int score;
            if (winner == turn) {
                score = BIG + 10000 * depth;
            } else if (winner == other) {
                score = -BIG - 10000 * depth;
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
            return 0;
        }
        return bestScore;
    }

};

#endif