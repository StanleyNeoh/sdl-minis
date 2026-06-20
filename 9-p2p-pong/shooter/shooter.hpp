#ifndef SHOOTER_HPP 
#define SHOOTER_HPP 

#include <array>
#include <SDL.h>
#include "lib/common/common.hpp"
#include "p2p/p2p.hpp"
#include "textures/textures.hpp"
#include "imgui.h"

namespace Shooter {

    struct Projectile {
        Vec2 size{5.0, 5.0};
        Vec2 pos{};
        Vec2 vel{};
        int player_id = -1;
        float life = -1;

        void render(SDL_Renderer* renderer) {
            if (life <= 0) return;
            SDL_FRect dest{
                pos.x - size.x / 2,
                pos.y - size.y / 2,
                size.x,
                size.y
            };
            SDL_RenderCopyExF(
                renderer,
                Textures::textures.get_circle_tex(),
                nullptr,
                &dest,
                0,
                nullptr,
                SDL_FLIP_NONE
            );
        }
    };
    struct Ship {
        int player_id;
        float trailfire_size = 20.0;
        Vec2 size{50.0, 50.0};
        Vec2 pos{500.0, 500.0};
        Vec2 vel{0.0, 0.0};
        float deg = 0;
        Uint64 last_shot_at = 0;

        Vec2 dir() {
            float rad = deg * (M_PI / 180.0f);
            return Vec2{sin(rad), -cos(rad)};
        }

        void render(SDL_Renderer* renderer, bool is_boosting) {
            SDL_FRect dst{
                pos.x - size.x / 2,
                pos.y - size.y / 2,
                size.x,
                size.y
            };
            SDL_RenderCopyExF(
                renderer,
                Textures::textures.get_ship_tex(),
                nullptr,
                &dst,
                deg,
                nullptr,
                SDL_FLIP_NONE
            );
            if (is_boosting) {
                Vec2 _dir = dir();
                SDL_FRect dst{
                    pos.x - size.x * _dir.x - trailfire_size / 2,
                    pos.y - size.y * _dir.y - trailfire_size / 2,
                    trailfire_size,
                    trailfire_size
                };
                SDL_RenderCopyExF(
                    renderer,
                    Textures::textures.get_trail_fire_tex(),
                    nullptr,
                    &dst,
                    deg,
                    nullptr,
                    SDL_FLIP_NONE
                );
            }
        }
    };

    struct Shooter {
        constexpr static int projectiles_size = 10;
        SDL_Texture* tex = nullptr;
        float width = 1000.0f;
        float height = 1000.0f;

        Ship ship1{1};
        size_t projectile_i = 0;
        std::array<Projectile, projectiles_size> projectiles;

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

        void process_sdl_event(const SDL_Event& event);

        void step(bool& is_boosting);

        void draw() {
            if (tex == nullptr) return;
            bool is_boosting;
            step(is_boosting);

            ImVec2 avail = ImGui::GetContentRegionAvail();
            float canvasSide = std::min(avail.x, avail.y);
            ImVec2 canvasSize = ImVec2{canvasSide, canvasSide};

            SDL_Renderer* renderer = Textures::textures.renderer;

            SDL_SetRenderTarget(renderer, tex);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            ship1.render(renderer, is_boosting);
            for (int i = 0; i < projectiles_size; i++) {
                projectiles[i].render(renderer);
            }

            SDL_SetRenderTarget(renderer, nullptr);

            ImGui::Image(tex, canvasSize);
        }
    };

    extern Shooter shooter;
}

#endif