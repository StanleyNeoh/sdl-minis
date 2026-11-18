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

    static constexpr int cp = 2;
    static constexpr int cd = 200;
    static constexpr int cr = cd / 2;
    static constexpr int ccr = cr - cp;

    static constexpr int innerW = cd * N;
    static constexpr int innerH = cd * M;

    static constexpr int br = 20;
    static constexpr int W = innerW + 2 * br;
    static constexpr int H = innerH + 2 * br;
    static constexpr float aspect_ratio = static_cast<float>(W) / (H);

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
                float tlx = cd * c + br;
                float tly = cd * r + br;
                float cx = tlx + cr;
                float cy = tly + cr;
                for (int y = tly; y < tly + cd; y++) {
                    for (int x = tlx; x < tlx + cd; x++) {
                        float dx = x - cx;
                        float dy = y - cy;
                        float d2 = dx * dx + dy * dy;
                        constexpr float thresh = cr - cp;
                        if (d2 > thresh * thresh) {
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
