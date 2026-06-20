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
        SDL_Color color;
        Vec2 size{10.0, 10.0};
        Vec2 pos{};
        Vec2 vel{};
        int player_id = -1;
        float life = -1;

        void reset() {
            life = -1;
        }

        void render(SDL_Renderer* renderer) {
            if (life <= 0) return;
            SDL_SetTextureColorMod(Textures::textures.get_circle_tex(), color.r, color.g, color.b);
            SDL_SetTextureBlendMode(Textures::textures.get_circle_tex(), SDL_BLENDMODE_BLEND);
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
        SDL_Color color;
        float trailfire_size = 20.0;
        Vec2 size{50.0, 50.0};
        Vec2 pos{500.0, 500.0};
        Vec2 vel{0.0, 0.0};
        float deg = 0;
        Uint64 last_shot_at = 0;
        bool is_boosting = false;

        Vec2 dir() {
            float rad = deg * (M_PI / 180.0f);
            return Vec2{sin(rad), -cos(rad)};
        }

        void reset() {
            pos.x = 500.0;
            pos.y = 500.0;
            vel.x = 0.0;
            vel.y = 0.0;
            deg = 0;
            last_shot_at = 0;
            is_boosting = false;
        }

        void render(SDL_Renderer* renderer) {
            SDL_SetTextureColorMod(Textures::textures.get_ship_tex(), color.r, color.g, color.b);
            SDL_SetTextureBlendMode(Textures::textures.get_ship_tex(), SDL_BLENDMODE_BLEND);
            std::cout << color.r << " " << color.g << " " << color.b << "\n";
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

        bool is_hit(Projectile& projectile) {
            if (projectile.life <= 0) return false;
            bool overlap_x = (
                pos.x - size.x / 2 < projectile.pos.x &&
                pos.x + size.x / 2 > projectile.pos.x
            );
            bool overlap_y = (
                pos.y - size.y / 2 < projectile.pos.y &&
                pos.y + size.y / 2 > projectile.pos.y
            );
            return overlap_x && overlap_y && projectile.player_id != player_id;
        }
    };

    struct Shooter {
        constexpr static int projectiles_size = 25;
        SDL_Texture* tex = nullptr;
        float width = 1000.0f;
        float height = 1000.0f;

        Ship ship1{1, {255, 0, 0}};
        Ship ship2{2, {0, 255, 0}};
        size_t projectile_i = 0;
        std::array<Projectile, projectiles_size> projectiles;
        int ship1_score = 0;
        int ship2_score = 0;

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

        void reset() {
            ship1_score = 0;
            ship2_score = 0;
            ship1.reset();
            ship2.reset();
            for (int i = 0; i < projectiles_size; i++) {
                projectiles[i].reset();
            }
        }

        void shoot(Ship& ship) {
            if (projectiles[projectile_i].life > 0) return;
            Uint64 now = SDL_GetTicks64();
            if (now - ship.last_shot_at < 100) return;
            auto dir = ship.dir();
            projectiles[projectile_i].color = ship.color;
            projectiles[projectile_i].life = 1.0;
            projectiles[projectile_i].pos.x = ship.pos.x + ship.size.x * dir.x;
            projectiles[projectile_i].pos.y = ship.pos.y + ship.size.y * dir.y;
            projectiles[projectile_i].vel.x = 1000.0 * dir.x;
            projectiles[projectile_i].vel.y = 1000.0 * dir.y;
            projectiles[projectile_i].player_id = ship.player_id;
            ship.last_shot_at = now;
            projectile_i = (projectile_i + 1) % projectiles_size;
        }

        void process_sdl_event(const SDL_Event& event);

        void step_projectile(Projectile& projectile, float dt);
        void step_ship(
            Ship& ship, 
            float dt,
            SDL_Scancode up_code,
            SDL_Scancode left_code,
            SDL_Scancode right_code
        );
        void step();

        void draw() {
            if (tex == nullptr) return;
            step();

            ImVec2 avail = ImGui::GetContentRegionAvail();
            float canvasSide = std::min(avail.x, avail.y);
            ImVec2 canvasSize = ImVec2{canvasSide, canvasSide};

            SDL_Renderer* renderer = Textures::textures.renderer;

            SDL_SetRenderTarget(renderer, tex);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            ship1.render(renderer);
            ship2.render(renderer);
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