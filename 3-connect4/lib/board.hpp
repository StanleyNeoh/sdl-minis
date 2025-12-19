#ifndef LIB_CONNECT4_HPP
#define LIB_CONNECT4_HPP

#include <iostream>
#include <vector>
#include <array>
#include <climits>
#include "viewport.hpp"
#include "utils.hpp"
#include "engine.hpp"

template <int M, int N>
struct BoardFrame: public ViewPort<BoardFrame<M, N>> {
    using Par = ViewPort<BoardFrame<M, N>>;
    using Par::rect;
    using Par::tex;
    using Par::format;
    using Par::draw_tex;
    static constexpr Color BACKGROUND{0, 0, 100};

    int br = 20;
    int bw = -1;
    int bh = -1;
    float cw = -1;
    float cwr = -1;
    float ch = -1;
    float chr = -1;

    BoardFrame(int br): br(br) {}
    BoardFrame() = default;

    bool init(SDL_Renderer* renderer, int x, int y, int w, int h) {
        if (!Par::init(renderer, x, y, w, h)) return false;
        if (!Par::init_tex(renderer, x, y, w, h)) return false;
        uint32_t* pixels;
        int pitch;
        if (SDL_LockTexture(tex, NULL, reinterpret_cast<void**>(&pixels), &pitch) != 0) {
            return false;
        }

        bw = rect.w - 2 * br;
        bh = rect.h - 2 * br;
        cw = static_cast<float>(bw) / N;
        cwr = cw / 2.0;
        ch = static_cast<float>(bh) / M;
        chr = ch / 2.0;
        uint32_t bgColor = map_color(format, BACKGROUND);

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
            for (int c = 0; c < br; c++) {
                rowpix[c] = bgColor;
            }
            for (int c = rect.w - br; c < rect.w; c++) {
                rowpix[c] = bgColor;
            }
        }

        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                float tlx = cw * c + br;
                float tly = ch * r + br;
                float cx = tlx + cwr;
                float cy = tly + chr;
                for (int y = tly; y < tly + ch; y++) {
                    uint32_t* rowpix = unsafe_shift(pixels, y * pitch);
                    for (int x = tlx; x < tlx + cw; x++) {
                        float dx = (x - cx) / cwr;
                        float dy = (y - cy) / chr;
                        float d2 = dx * dx + dy * dy;
                        if (d2 > 0.8) {
                            rowpix[x] = bgColor;
                        } 
                    }
                }
            }
        }
        SDL_UnlockTexture(tex);
        return true;
    }

    bool draw(SDL_Renderer* renderer) {
        return draw_tex(renderer, tex);
    }
};

struct BoardCell: public ViewPort<BoardCell> {
    using Par = ViewPort<BoardCell>;
    using Par::tex;
    using Par::format;
    using Par::rect;

    static constexpr Color hl{0, 255, 0};

    Cell& ref;
    Color color;
    int final_y;

    BoardCell(Color color, int final_y, Cell& ref): Par(), color(color), final_y(final_y), ref(ref) {}

    bool init(SDL_Renderer* renderer, int x, int y, int w, int h) {
        if (!Par::init(renderer, x, y, w, h)) return false;
        if (!init_tex(renderer, x, y, w, h)) return false;
        if (!update_tex()) return false;
        return true;
    }

    bool update_tex() {
        uint32_t* pixels;
        int pitch;
        if (SDL_LockTexture(tex, NULL, reinterpret_cast<void**>(&pixels), &pitch) != 0) {
            return false;
        }
        float cwr = rect.w / 2.0;
        float chr = rect.h / 2.0;
        uint32_t cell_color = map_color(format, color);
        uint32_t hl_color = map_color(format, hl);

        for (int r = 0; r < rect.h; r++) {
            uint32_t* rowpix = unsafe_shift(pixels, r * pitch);
            for (int c = 0; c < rect.w; c++) {
                float dy = (r - chr) / chr;
                float dx = (c - cwr) / cwr;
                float d2 = dx * dx + dy * dy;
                if (ref.is_marked() && d2 < 0.05) {
                    rowpix[c] = hl_color;
                } else if (d2 <= 1.0) {
                    rowpix[c] = cell_color;
                }
            }
        }
        SDL_UnlockTexture(tex);
        return true;
    }

    bool draw(SDL_Renderer* renderer) {
        return draw_tex(renderer, tex);
    }

    bool step() {
        if (rect.y == final_y) return false;
        rect.y = std::min(final_y, rect.y + 10);
        return true;
    }
};

template <int M, int N>
struct Board: public ViewPort<Board<M, N>> {
    using Par = ViewPort<Board<M, N>>;
    using Par::rect;
    using Par::tex;
    using Par::format;
    
    Grid<M, N> grid;

    std::vector<BoardCell> cells;
    BoardFrame<M, N> boardframe;

    SDL_Renderer* renderer;
    int col_i = -1;
    int turn = PlayerRedKey;
    int winner = PlayerNoneKey;

    bool init(SDL_Renderer* renderer, int x, int y, int w, int h) {
        if (!Par::init(renderer, x, y, w, h)) return false;
        if (!boardframe.init(renderer, x, y, w, h)) return false;
        this->renderer = renderer;
        return true;
    }

    bool draw(SDL_Renderer* renderer) {
        for (auto& cell: cells) {;
            if (!cell.draw(renderer)) return false;
            cell.step();
        }
        return boardframe.draw(renderer);
    }

    void handle_mouse_motion(const SDL_MouseMotionEvent& e) {
        int cw = (rect.w - 2 * boardframe.br) / N;
        int ind =  (e.x - boardframe.br) / cw; 
        if (ind < 0 || ind >= N) {
            col_i = -1;
        } else {
            col_i = ind;
        };
    }

    void handle_mouse_up(const SDL_MouseButtonEvent& e) {
        if (!drop_piece(col_i, PlayerRedKey, 0)) return;
        winner = grid.check_win();
        if (check_win()) return;

        int bot_col_i = grid.bot_plays(PlayerBlueKey);
        if (!drop_piece(bot_col_i, PlayerBlueKey, -100)) return;
        check_win();
    }

    bool check_win() {
        winner = grid.check_win();
        if (winner != PlayerNoneKey) {
            std::cout << "Winner: " << winner << "\n";
            for (auto& cell: cells) {
                cell.update_tex();
            }
            return true;
        } 
        return false;
    }

    bool drop_piece(int col_i, CellKey turn, int start_y = 0) {
        if (winner != PlayerNoneKey) return false;
        int r = grid.drop_piece(col_i, turn);
        if (r < 0) return false;
        float cw = boardframe.cw;
        float ch = boardframe.ch;
        float tlx = + cw * col_i + boardframe.br;
        float tly = + ch * r + boardframe.br;
        switch(turn) {
        case PlayerBlueKey:
            cells.push_back(BoardCell({0,0,255}, tly, grid.get(r, col_i)));
            cells.back().init(this->renderer, tlx, start_y, cw, ch);
            return true;
        case PlayerRedKey:
            cells.push_back(BoardCell({255,0,0}, tly, grid.get(r, col_i)));
            cells.back().init(this->renderer, tlx, start_y, cw, ch);
            return true;
        }
        return false;
    };
};

#endif