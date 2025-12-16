#ifndef LIB_CONNECT4_HPP
#define LIB_CONNECT4_HPP

#include <iostream>
#include <array>
#include "viewport.hpp"
#include "utils.hpp"

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

    bool operator==(const CellKey& k) {
        return key == k;
    }

    bool operator!=(const CellKey& k) {
        return key != k;
    }

    void mark() {
        marked = true;
    }

    void unmark() {
        marked = false;
    }

    bool isMarked() {
        return marked;
    }

    uint32_t getSDLColor(SDL_PixelFormat* format) {
        uint32_t color;
        switch(key) {
        case PlayerBlueKey:
            return mapColor(format, BLUE);
            break;
        case PlayerRedKey:
            return mapColor(format, RED);
            break;
        default:
            return mapColor(format, WHITE);
            break;
        };
    }
};


template <int M, int N>
struct Board: public ViewPort<Board<M, N>> {
    using Par = ViewPort<Board<M, N>>;
    using Par::rect;
    using Par::tex;
    using Par::format;
    using Grid = std::array<std::array<Cell, N>, M>;

    static constexpr Color BACKGROUND{0, 0, 100};
    static constexpr Color HIGHLIGHT{0, 255, 0};
    
    Grid grid;
    int br = 20;
    Cell::CellKey turn = Cell::PlayerRedKey;
    Cell::CellKey winner = Cell::PlayerNoneKey;
    int col_i = -1;

    Board() {
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                grid[r][c] = Cell();
            }
        }
    }

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
        uint32_t bgColor = mapColor(format, BACKGROUND);
        uint32_t hlColor = mapColor(format, HIGHLIGHT);

        for (int r = 0; r < br; r++) {
            int dy = br - r;
            int dx = br - SDL_sqrt(br * br - dy * dy);
            uint32_t* toppad = unsafeShift(pixels, r * pitch);
            uint32_t* botpad = unsafeShift(pixels, (rect.h - 1 - r) * pitch);
            for (int c = dx; c < rect.w - dx; c++) {
                botpad[c] = toppad[c] = bgColor;
            }
        }
        for (int r = br; r < rect.h - br; r++) {
            uint32_t* rowpix = unsafeShift(pixels, r * pitch);
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
                uint32_t pColor = grid[r][c].getSDLColor(format);
                bool isMarked = grid[r][c].isMarked();
                for (int y = tly; y < tly + ch; y++) {
                    uint32_t* rowpix = unsafeShift(pixels, y * pitch);
                    for (int x = tlx; x < tlx + cw; x++) {
                        float dx = (x - cx) / cwr;
                        float dy = (y - cy) / chr;
                        float d = dx * dx + dy * dy;
                        if (d <= 0.8) {
                            rowpix[x] = pColor;
                        } else if (isMarked && d <= 0.9) {
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
        drop_piece(col_i);
        winner = check_win();
    }

    void switch_turn() {
        switch(turn) {
        case Cell::PlayerBlueKey:
            turn = Cell::PlayerRedKey;
            break;
        case Cell::PlayerRedKey:
            turn = Cell::PlayerBlueKey;
            break;
        default:
            break;
        }
    }

    bool drop_piece(int col_i) {
        for (int r = M-1; r >= 0; r--) {
            if (grid[r][col_i] != Cell::PlayerNoneKey) continue;
            grid[r][col_i].set(turn);
            std::cout << grid << "\n";
            switch_turn();
            return true;
        }
        return false;
    }

    Cell::CellKey check_win(int req = 4) {
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
        // 0 <= r = c + i <= min(N-1+i, M-1)
        for (int i = -N+1; i <= M-1; i++) {
            Cell::CellKey pp = Cell::PlayerNoneKey;
            int pc = 0;
            for (int r = 0; r <= std::min(N-1+i, M-1); r++) {
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
            for (int r = 0; r <= i; r++) {
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

    private:
        bool check_loop(int req, Cell p, Cell::CellKey& pp, int& pc) {
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
        }
};

#endif