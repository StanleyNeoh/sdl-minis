#ifndef LIB_TEXTURES_BOARDCELL_HPP
#define LIB_TEXTURES_BOARDCELL_HPP

#include "SDL.h"

namespace Textures {
namespace BoardCell {
    SDL_Texture* tex = NULL;

    constexpr int W = 1000;
    constexpr int H = 1000;
    constexpr float wr = static_cast<float>(W) / 2.0;
    constexpr float hr = static_cast<float>(H) / 2.0;

    constexpr u_int32_t RED = 0xFFFF0000;
    constexpr u_int32_t BLUE = 0xFF0000FF;
    constexpr u_int32_t HIGHLIGHT = 0xFF00FF00;

    bool init(SDL_Renderer* renderer) {
        if (tex != NULL) return false;

        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        u_int32_t pix[H][W] = {0};
        for (int r = 0; r < H; r++) {
            float dr = (r - hr) / hr;
            float dr2 = dr * dr;
            for (int c = 0; c < W; c++) {
                float dc = (c - wr) / wr;
                float d2 = dc * dc + dr2;
                if (d2 < 1.0) {
                    pix[r][c] = 0xFFFFFFFF;
                }
            }
        }
        SDL_UpdateTexture(tex, NULL, &pix, W * sizeof(u_int32_t));
        return true;
    }
}
}

#endif