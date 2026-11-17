#ifndef TEXTURES_CIRCLE_HPP
#define TEXTURES_CIRCLE_HPP

#include <SDL.h>
#include "../context.hpp"


namespace Textures {

struct Circle {
    inline static SDL_Texture* tex = NULL;

    static constexpr int W = 1000;
    static constexpr int H = 1000;
    static constexpr float wr = static_cast<float>(W) / 2.0;
    static constexpr float hr = static_cast<float>(H) / 2.0;

    static void load() {
        if (tex != NULL) return;
        using Context::renderer;

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
                    pix[r][c] = 0XFFFFFFFF;
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

}

#endif
