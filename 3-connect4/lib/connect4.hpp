#ifndef LIB_CONNECT4_HPP
#define LIB_CONNECT4_HPP

#include <iostream>
#include <array>
#include "viewport.hpp"
#include "utils.hpp"

#define BIG 1000000

struct Cell {
    using CellKey = char;
    constexpr static CellKey PlayerNoneKey = '.';
    constexpr static CellKey PlayerBlueKey = 'B';
    constexpr static CellKey PlayerRedKey = 'R';
    constexpr static Color WHITE{255, 255, 255};
    constexpr static Color RED{255, 0, 0};
    constexpr static Color BLUE{0, 0, 255};

    char key;
    bool marked;

    Cell(): Cell(PlayerNoneKey) {}
    Cell(char c): key(c), marked(false) {}

    static CellKey other_player(CellKey key) {
        switch(key) {
        case Cell::PlayerBlueKey:
            return Cell::PlayerRedKey;
        case Cell::PlayerRedKey:
            return Cell::PlayerBlueKey;
        default:
            return Cell::PlayerNoneKey;
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

    uint32_t get_sdl_color(SDL_PixelFormat* format) {
        uint32_t color;
        switch(key) {
        case PlayerBlueKey:
            return map_color(format, BLUE);
            break;
        case PlayerRedKey:
            return map_color(format, RED);
            break;
        default:
            return map_color(format, WHITE);
            break;
        };
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

    bool drop_piece(int col_i, Cell::CellKey turn) {
        for (int r = M-1; r >= 0; r--) {
            if (grid[r][col_i] != Cell::PlayerNoneKey) continue;
            grid[r][col_i].set(turn);
            return true;
        }
        return false;
    }

    bool undrop_piece(int col_i) {
        for (int r = 0; r < M; r++) {
            if (grid[r][col_i] == Cell::PlayerNoneKey) continue;
            grid[r][col_i].set(Cell::PlayerNoneKey);
            return true;
        }
        return false;
    }

    Cell::CellKey check_win(int req = 4) {
        auto check_loop = [](int req, Cell p, Cell::CellKey& pp, int& pc) {
            if (p.key == pp) {
                pc++;
            } else {
                pp = p.key;
                pc = 1;
            }
            if (pc >= req && pp != Cell::PlayerNoneKey) {
                return true;
            }
            return false;
        };

        // Check row
        for (int r = 0; r < M; r++) {
            Cell::CellKey pp = Cell::PlayerNoneKey;
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
            Cell::CellKey pp = Cell::PlayerNoneKey;
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
            Cell::CellKey pp = Cell::PlayerNoneKey;
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
            Cell::CellKey pp = Cell::PlayerNoneKey;
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
        return Cell::PlayerNoneKey;
    }

    Cell& get(int r, int c) {
        return grid[r][c];
    }

    int count_winning_spots(Cell::CellKey key, int req = 4) {
        static const std::array<std::pair<int, int>, 8> steps{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}}};
        int count = 0;
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                if (grid[r][c].key != Cell::PlayerNoneKey) continue;
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

    Action search(Cell::CellKey turn, int depth = 7) {
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

private:
    int heuristic(Cell::CellKey turn) {
        Cell::CellKey other = Cell::other_player(turn);
        int score = 0;
        score += count_winning_spots(turn, 4) * 100;
        score -= count_winning_spots(other, 4) * 100;
        score += count_winning_spots(turn, 3);
        score -= count_winning_spots(other, 3);
        return score;
    }

    int search_AB(Cell::CellKey turn, Action* action = nullptr, int depth = 7, int a = -BIG, int b = BIG) {
        if (depth == 0) return heuristic(turn);
        int pos = count_winning_spots(turn, 4);
        int neg = count_winning_spots(other, 4);
        Cell::CellKey other = Cell::other_player(turn);
        int bestScore = INT_MIN;
        int bestMove = -1;
        for (int c = 0; c < N; c++) {
            if (!drop_piece(c, turn)) continue;
            Cell::CellKey winner = check_win();
            float score;
            if (winner == turn) {
                score = BIG + 1000 * depth;
            } else if (winner == other) {
                score = -BIG - 1000 * depth;
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
        return bestScore;
    }

};

template <int M, int N>
struct Board: public ViewPort<Board<M, N>> {
    using Par = ViewPort<Board<M, N>>;
    using Par::rect;
    using Par::tex;
    using Par::format;

    static constexpr Color BACKGROUND{0, 0, 100};
    static constexpr Color HIGHLIGHT{0, 255, 0};
    
    Grid<M, N> grid;
    int br = 20;
    Cell::CellKey turn = Cell::PlayerRedKey;
    Cell::CellKey winner = Cell::PlayerNoneKey;
    int col_i = -1;

    bool draw(SDL_Renderer* renderer) {
        uint32_t* pixels;
        int pitch;
        if (SDL_LockTexture(tex, NULL, reinterpret_cast<void**>(&pixels), &pitch) != 0) {
            return false;
        }
        int bw = rect.w - 2 * br;
        int bh = rect.h - 2 * br;
        int cw = bw / N;
        float cwr = cw / 2.0;
        int ch = bw / M;
        float chr = ch / 2.0;
        uint32_t bgColor = map_color(format, BACKGROUND);
        uint32_t hlColor = map_color(format, HIGHLIGHT);

        for (int r = 0; r < br; r++) {
            int dy = br - r;
            int dx = br - SDL_sqrt(br * br - dy * dy);
            uint32_t* toppad = unsafe_shift(pixels, r * pitch);
            uint32_t* botpad = unsafe_shift(pixels, (rect.h - 1 - r) * pitch);
            for (int c = dx; c < rect.w - dx; c++) {
                botpad[c] = toppad[c] = bgColor;
            }
        }
        for (int r = br; r < rect.h - br; r++) {
            uint32_t* rowpix = unsafe_shift(pixels, r * pitch);
            for (int c = 0; c < rect.w; c++) {
                rowpix[c] = bgColor;
            }
        }
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                int tlx = br + cw * c;
                int tly = br + ch * r;
                float cx = tlx + cwr;
                float cy = tly + chr;
                uint32_t pColor = grid.get(r, c).get_sdl_color(format);
                bool is_marked = grid.get(r, c).is_marked();
                for (int y = tly; y < tly + ch; y++) {
                    uint32_t* rowpix = unsafe_shift(pixels, y * pitch);
                    for (int x = tlx; x < tlx + cw; x++) {
                        float dx = (x - cx) / cwr;
                        float dy = (y - cy) / chr;
                        float d = dx * dx + dy * dy;
                        if (d <= 0.8) {
                            rowpix[x] = pColor;
                        } else if (is_marked && d <= 0.9) {
                            rowpix[x] = hlColor;
                        }
                    }
                }
            }
        }
        SDL_UnlockTexture(tex);
        return Par::draw(renderer);
    }

    void handle_mouse_motion(const SDL_MouseMotionEvent& e) {
        int cw = (rect.w - 2 * br) / N;
        int ind =  (e.x - br) / cw; 
        if (ind < 0 || ind >= N) {
            col_i = -1;
        } else {
            col_i = ind;
        };
    }

    void handle_mouse_up(const SDL_MouseButtonEvent& e) {
        if (winner != Cell::PlayerNoneKey) return;
        if (!grid.drop_piece(col_i, turn)) return;
        handover_turn();
        bot_plays();
    }

    void handover_turn() {
        winner = grid.check_win();
        if (winner != Cell::PlayerNoneKey) {
            std::cout << "Winner: " << winner << "\n";
        } else {
            int blue_c = grid.count_winning_spots(Cell::PlayerBlueKey);
            int red_c = grid.count_winning_spots(Cell::PlayerRedKey);
            std::cout << "num3blue: " << blue_c << ", num3red: " << red_c << "\n";
        }
        std::cout << grid << "\n";
        turn = Cell::other_player(turn);
    }

    void bot_plays() {
        auto action = grid.search(turn);
        std::cout << "Bot: " << action.bestMove << "\n";
        for (int score: action.scores) {
            std::cout << score << ", ";
        }
        std::cout << "\n" << grid << "\n";
        grid.drop_piece(action.bestMove, turn);
        handover_turn();
    }
};

#endif