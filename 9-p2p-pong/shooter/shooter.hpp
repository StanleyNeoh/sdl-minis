#ifndef SHOOTER_HPP 
#define SHOOTER_HPP 

#include <array>
#include <SDL.h>
#include "lib/common/utils.hpp"
#include "imgui.h"

namespace Shooter {
    struct Shooter {
        Rand::Perlin2D<63, 63> perlin;
        SDL_Texture* tex = nullptr;

        Shooter(float width = 1000, float height = 1000): perlin(width, height) {}

        void initialise(SDL_Renderer* renderer) {
            if (tex != nullptr) return;
            tex = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                perlin.width,
                perlin.height
            );
        }

        void draw() {
            if (tex == nullptr) return;
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float canvasSide = std::min(avail.x, avail.y);
            ImVec2 canvasSize(canvasSide, canvasSide);

            void* pix = nullptr;
            int pitch = 0;
            if (SDL_LockTexture(tex, nullptr, &pix, &pitch)) return;
            for (int r = 0; r < perlin.height; r++) {
                char* row_start = reinterpret_cast<char*>(pix) + r * pitch;
                for (int c = 0; c < perlin.width; c++) {
                    float n = perlin.query(r, c);
                    uint8_t v = (n * 0.5f + 0.5f) * 255;
                    reinterpret_cast<uint32_t*>(row_start)[c] = (255 << 24) | (v << 16) | (v << 8) | v;
                }
            }
            SDL_UnlockTexture(tex);
            ImGui::Image(tex, canvasSize);
            perlin.reset();
        }
    };

    extern Shooter shooter;
}

#endif