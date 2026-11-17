#ifndef TEXTURES_CONNECT4_BOARDFRAME_HPP
#define TEXTURES_CONNECT4_BOARDFRAME_HPP

#include <SDL.h>
#include "../../context.hpp"
#include "../circle.hpp"
#include <iostream>

namespace Textures {
namespace Connect4 {

struct BoardFrame {
    inline static SDL_Texture* tex = NULL;

    static constexpr int M = 6;
    static constexpr int N = 7;

    static constexpr int W = 1000;
    static constexpr int H = 1000;

    static constexpr int br = 20;
    static constexpr int bw = W - 2 * br;
    static constexpr int bh = H - 2 * br;

    static constexpr int cbr = 2;
    static constexpr int cw = static_cast<float>(bw) / N;
    static constexpr int cwr = cw / 2.0;
    static constexpr int ch = static_cast<float>(bh) / M;
    static constexpr int chr = ch / 2.0;

    static constexpr u_int32_t BG_COLOR = 0xFFFFFFFF;

    static void load() {
        if (tex != NULL) return;
        using Context::renderer;

        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        u_int32_t pix[H][W] = {0};
        for (int r = 0; r < br; r++) {
            int dy = br - r;
            int dx = br - SDL_sqrt(br * br - dy * dy);
            for (int c = dx; c < W - dx; c++) {
                pix[r][c] = pix[H-1-r][c] = BG_COLOR;
            }
        }

        for (int r = br; r < H - br; r++) {
            for (int c = 0; c < br; c++) {
                pix[r][c] = BG_COLOR;
            }
            for (int c = W - 1 - br; c < W; c++) {
                pix[r][c] = BG_COLOR;
            }
        }

        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                float tlx = cw * c + br;
                float tly = ch * r + br;
                float cx = tlx + cwr;
                float cy = tly + chr;
                for (int y = tly; y < tly + ch; y++) {
                    for (int x = tlx; x < tlx + cw; x++) {
                        float dx = (x - cx) / (cwr - 2 * cbr);
                        float dy = (y - cy) / (chr - 2 * cbr);
                        float d2 = dx * dx + dy * dy;
                        if (d2 > 1.0) {
                            pix[y][x] = BG_COLOR;
                        } 
                    }
                }
            }
        }

        SDL_UpdateTexture(tex, NULL, &pix, W * sizeof(u_int32_t));
    }

    static SDL_Texture* get() {
        load();
        return tex;
    }
};

}}

#endif
