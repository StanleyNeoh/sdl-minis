#ifndef LIB_CONNECT4_HPP
#define LIB_CONNECT4_HPP

#include <iostream>
#include <array>
#include "viewport.hpp"
#include "utils.hpp"

template <int M, int N>
struct Board: public ViewPort<Board<M, N>> {
    using Par = ViewPort<Board<M, N>>;
    using Par::rect;
    using Par::tex;
    using Par::format;
    using Grid = std::array<std::array<Player, N>, M>;
    
    Grid grid;
    int br = 20;
    Player turn = Player::PlayerRed;
    Player winner = Player::None;
    int col_i = -1;

    Board() {
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                grid[r][c] = Player::None;
            }
        }
    }

    bool draw_board(int br, Color bc) {
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
        uint32_t color = map_color(format, bc);

        for (int r = 0; r < br; r++) {
            int dy = br - r;
            int dx = br - SDL_sqrt(br * br - dy * dy);
            uint32_t* toppad = unsafe_shift(pixels, r * pitch);
            uint32_t* botpad = unsafe_shift(pixels, (rect.h - 1 - r) * pitch);
            for (int c = dx; c < rect.w - dx; c++) {
                botpad[c] = toppad[c] = color;
            }
        }
        for (int r = br; r < rect.h - br; r++) {
            uint32_t* rowpix = unsafe_shift(pixels, r * pitch);
            for (int c = 0; c < rect.w; c++) {
                rowpix[c] = color;
            }
        }
        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                int tlx = br + cw * c;
                int tly = br + ch * r;
                float cx = tlx + cwr;
                float cy = tly + chr;
                uint32_t color;
                switch(grid[r][c]) {
                case Player::PlayerBlue:
                    color = map_color(format, BLUE);
                    break;
                case Player::PlayerRed:
                    color = map_color(format, RED);
                    break;
                default:
                    color = map_color(format, WHITE);
                    break;
                };
                for (int y = tly; y < tly + ch; y++) {
                    uint32_t* rowpix = unsafe_shift(pixels, y * pitch);
                    for (int x = tlx; x < tlx + cw; x++) {
                        float dx = (x - cx) / cwr;
                        float dy = (y - cy) / chr;
                        if (dx * dx + dy * dy <= 0.8) {
                            rowpix[x] = color;
                        }
                    }
                }
            }
        }
        SDL_UnlockTexture(tex);
        return true;
    }

    bool draw(SDL_Renderer* renderer) {
        draw_board(br, BOARD_BG);
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
        std::cout << "Click " <<  col_i << std::endl;
        drop_piece(col_i);
    }

    void switch_turn() {
        switch(turn) {
        case Player::PlayerRed:
            turn = Player::PlayerBlue;
            break;
        case Player::PlayerBlue:
            turn = Player::PlayerRed;
            break;
        }
        std::cout << "Switched to " << turn << "\n";
    }

    bool drop_piece(int col_i) {
        for (int r = M-1; r >= 0; r--) {
            if (grid[r][col_i] != Player::None) continue;
            grid[r][col_i] = turn;
            std::cout << grid <<  "=> " << r << " " << col_i << " " << turn << "\n";
            switch_turn();
            return true;
        }
        return false;
    }

    Player check_win(int req = 4) {
        // Check row
        for (int r = 0; r < M; r++) {
            Player pp = Player::None;
            int pc = 0;
            for (int c = 0; c < N; c++) {
                Player p = grid[r][c];
                if (p == pp) {
                    pc++;
                } else {
                    pp = p;
                    pc = 1;
                }
                if (pc >= req && pp != Player::None) {
                    return pp;
                }
            }
        }

        // Check col
        for (int c = 0; c < N; c++) {
            Player pp = Player::None;
            int pc = 0;
            for (int r = 0; r < M; r++) {
                Player p = grid[r][c];
                if (p == pp) {
                    pc++;
                } else {
                    pp = p;
                    pc = 1;
                }
                if (pc >= req && pp != Player::None) {
                    return pp;
                }
            }
        }
        return false;
    }
};

#endif