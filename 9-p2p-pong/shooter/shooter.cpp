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
                        shoot(ship1);
                        break;
                    }
                    case SDLK_RSHIFT: {
                        shoot(ship2);
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


    void Shooter::step_projectile(Projectile& projectile, float dt) {
        if (projectile.life <= 0) return;
        projectile.pos.x += projectile.vel.x * dt;
        projectile.pos.y += projectile.vel.y * dt;
        projectile.life -= dt;
        while (projectile.pos.x < 0) projectile.pos.x += width;
        while (projectile.pos.y < 0) projectile.pos.y += height;
        while (projectile.pos.x > width) projectile.pos.x -= width;
        while (projectile.pos.y > height) projectile.pos.y -= height;
    }

    void Shooter::step_ship(
        Ship& ship, 
        float dt, 
        SDL_Scancode up_code,
        SDL_Scancode left_code,
        SDL_Scancode right_code
    ) {
        ship.is_boosting = false;
        float scale = 300.0 * dt;
        Vec2 dir = ship.dir();
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[left_code]) {
            ship.deg -= scale;
        }
        if (state[right_code]) {
            ship.deg += scale;
        }
        if (state[up_code]) {
            ship.vel.y += dir.y * scale;
            ship.vel.x += dir.x * scale;
            ship.is_boosting = true;
        }
        if (ship.vel.l2() >= 300.0 * 300.0) {
            ship.vel.normalise(300.0);
        }
        ship.pos.x += ship.vel.x * dt;
        ship.pos.y += ship.vel.y * dt;

        while (ship.pos.x < 0) ship.pos.x += width;
        while (ship.pos.y < 0) ship.pos.y += height;
        while (ship.pos.x > width) ship.pos.x -= width;
        while (ship.pos.y > height) ship.pos.y -= height;
    }

    void Shooter::step() {
        float dt = App::app.frame_stopwatch.delta() / 1000.0f;
        step_ship(ship1, dt, SDL_SCANCODE_W, SDL_SCANCODE_A, SDL_SCANCODE_D);
        step_ship(ship2, dt, SDL_SCANCODE_UP, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
        for (int i = 0; i < projectiles_size; i++) {
            step_projectile(projectiles[i], dt);
        }
    }
}