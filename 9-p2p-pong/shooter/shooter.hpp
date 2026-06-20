#ifndef SHOOTER_HPP 
#define SHOOTER_HPP 

#include <array>
#include <SDL.h>
#include "lib/common/common.hpp"
#include "p2p/p2p.hpp"
#include "textures/textures.hpp"
#include "imgui.h"

namespace Shooter {
    struct Ship {
        Vec2 size{50.0, 50.0};
        Vec2 pos{1000.0, 1000.0};
        Vec2 vel{0.0, 0.0};
        float deg = 0;
    };

    struct Shooter {
        SDL_Texture* tex = nullptr;
        float width = 1000.0f;
        float height = 1000.0f;

        Ship ship1;
        void initialise(SDL_Renderer* renderer) {
            if (tex != nullptr) return;
            tex = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_TARGET,
                width,
                height
            );
            Textures::textures.initialise(renderer);
        }

        void process_takedown();

        void draw() {
            if (tex == nullptr) return;
            process_takedown();

            ImVec2 avail = ImGui::GetContentRegionAvail();
            float canvasSide = std::min(avail.x, avail.y);
            ImVec2 canvasSize = ImVec2{canvasSide, canvasSide};

            SDL_Renderer* renderer = Textures::textures.renderer;

            SDL_SetRenderTarget(renderer, tex);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            
            SDL_FRect dst{
                ship1.pos.x - ship1.size.x / 2,
                ship1.pos.y - ship1.size.y / 2,
                ship1.size.x,
                ship1.size.y
            };
            SDL_RenderCopyExF(
                renderer,
                Textures::textures.get_ship_tex(),
                nullptr,
                &dst,
                ship1.deg,
                nullptr,
                SDL_FLIP_NONE
            );
            SDL_SetRenderTarget(renderer, nullptr);

            ImGui::Image(tex, canvasSize);
        }
    };

    extern Shooter shooter;
}

#endif