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
        u_int32_t pix[height][width] = {0};
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
        SDL_UpdateTexture(ship_tex, NULL, &pix, width * sizeof(u_int32_t));
        return ship_tex;
    }

    SDL_Texture* trail_fire_tex = nullptr;
    SDL_Texture* Textures::get_trail_fire_tex() {
        if (trail_fire_tex != nullptr) return trail_fire_tex;
        constexpr int width = 100;
        constexpr int height = 100;
        trail_fire_tex = SDL_CreateTexture(
            renderer, 
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            width,
            height 
        );
        u_int32_t pix[height][width] = {0};
        for (int i = 0; i < height; i++) {
            float g = 0.5 * width / height;
            float lo = 0.5 * width - g * (height - i);
            float hi = 0.5 * width + g * (height - i);
            for (int j = 0; j < width; j++) {
                if (j >= lo && j <= hi) {
                    pix[i][j] = 0xFFFFAA00u;
                } else {
                    pix[i][j] = 0x00000000u;
                }
            }
        }
        SDL_UpdateTexture(trail_fire_tex, NULL, &pix, width * sizeof(u_int32_t));
        return trail_fire_tex;
    }

    SDL_Texture* circle_tex = nullptr;
    SDL_Texture* Textures::get_circle_tex() {
        if (circle_tex != nullptr) return circle_tex;
        constexpr int width = 100;
        constexpr int height = 100;
        circle_tex = SDL_CreateTexture(
            renderer, 
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            width,
            height 
        );
        u_int32_t pix[height][width] = {0};

        float cy = height / 2.0f;
        float cx = width / 2.0f;
        for (int i = 0; i < height; i++) {
            float dy = i - cy;
            float dy2 = (dy * dy) / (cy * cy);
            for (int j = 0; j < width; j++) {
                float dx = j - cx;
                float dx2 = (dx * dx) / (cx * cx);
                if (dx2 + dy2 <= 1) {
                    pix[i][j] = 0xFFFFFFFFu;
                } else {
                    pix[i][j] = 0x00000000u;
                }
            }
        }
        SDL_UpdateTexture(circle_tex, NULL, &pix, width * sizeof(u_int32_t));
        return circle_tex;
    }
}