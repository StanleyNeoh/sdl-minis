#include "shooter.hpp"
#include "p2p/p2p.hpp"
#include "app/app.hpp"
namespace Shooter {
    Shooter shooter;

    void Shooter::process_sdl_event(const SDL_Event& event) {
        switch (event.type) {
            case SDL_KEYDOWN: {
                auto key = event.key.keysym.sym;
                switch (key) {
                    case SDLK_SPACE: {
                        if (projectiles[projectile_i].life > 0) break;
                        Uint64 now = SDL_GetTicks64();
                        if (now - ship1.last_shot_at < 100) break;
                        auto dir = ship1.dir();
                        projectiles[projectile_i].life = 5.0;
                        projectiles[projectile_i].pos.x = ship1.pos.x + ship1.size.x * dir.x;
                        projectiles[projectile_i].pos.y = ship1.pos.y + ship1.size.y * dir.y;
                        projectiles[projectile_i].vel.x = 1000.0 * dir.x;
                        projectiles[projectile_i].vel.y = 1000.0 * dir.y;
                        projectiles[projectile_i].player_id = ship1.player_id;
                        ship1.last_shot_at = now;
                        projectile_i = (projectile_i + 1) % projectiles_size;
                        break;
                    }
                    default:
                        break;
                }
            }
            default:
                break;
        }
    }


    void Shooter::step(bool& is_boosting) {
        is_boosting = false;
        float dt = App::app.frame_stopwatch.delta() / 1000.0f;
        float scale = 300.0 * dt;
        Vec2 dir = ship1.dir();
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_A]) {
            ship1.deg -= scale;
        }
        if (state[SDL_SCANCODE_D]) {
            ship1.deg += scale;
        }
        if (state[SDL_SCANCODE_W]) {
            ship1.vel.y += dir.y * scale;
            ship1.vel.x += dir.x * scale;
            is_boosting = true;
        }
        if (ship1.vel.l2() >= 300.0 * 300.0) {
            ship1.vel.normalise(300.0);
        }
        ship1.pos.x += ship1.vel.x * dt;
        ship1.pos.y += ship1.vel.y * dt;

        while (ship1.pos.x < 0) ship1.pos.x += width;
        while (ship1.pos.y < 0) ship1.pos.y += height;
        while (ship1.pos.x > width) ship1.pos.x -= width;
        while (ship1.pos.y > height) ship1.pos.y -= height;

        for (int i = 0; i < projectiles_size; i++) {
            Projectile& projectile = projectiles[i];
            if (projectile.life <= 0) continue;
            projectile.pos.x += projectile.vel.x * dt;
            projectile.pos.y += projectile.vel.y * dt;
            projectile.life -= dt;
            while (projectile.pos.x < 0) projectile.pos.x += width;
            while (projectile.pos.y < 0) projectile.pos.y += height;
            while (projectile.pos.x > width) projectile.pos.x -= width;
            while (projectile.pos.y > height) projectile.pos.y -= height;
        }
    }
}