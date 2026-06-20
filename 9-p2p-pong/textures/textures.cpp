#include "textures.hpp"
#include "SDL.h"

namespace Textures {
    Textures textures;

    SDL_Texture* ship_tex = nullptr;
    SDL_Texture* Textures::get_ship_tex() {
        if (ship_tex != nullptr) return ship_tex;
        constexpr int width = 100;
        constexpr int height = 100;
        ship_tex = SDL_CreateTexture(
            renderer, 
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            width,
            height 
        );
        u_int32_t pix[width][height] = {0};
        for (int i = 0; i < height; i++) {
            float g = 0.5 * width / height;
            float lo = 0.5 * width - g * i;
            float hi = 0.5 * width + g * i;
            for (int j = 0; j < width; j++) {
                if (j >= lo && j <= hi) {
                    pix[i][j] = 0xFFFFFFFFu;
                } else {
                    pix[i][j] = 0x00000000u;
                }
            }
        }
        constexpr float cutoff_ratio = 0.7f;
        const int cutoff = static_cast<int>(height * cutoff_ratio);
        for (int i = cutoff; i < 100; i++) {
            float t = (i - cutoff) / static_cast<float>(height - cutoff);
            float g = 0.5 * width;
            float lo = 0.5 * width - g * t;
            float hi = 0.5 * width + g * t;
            for (int j = 0; j < 100; j++) {
                if (j >= lo && j <= hi) {
                    pix[i][j] = 0x00000000u;
                }
            }
        }
        SDL_UpdateTexture(ship_tex, NULL, &pix, 100 * sizeof(u_int32_t));
        return ship_tex;
    }
}