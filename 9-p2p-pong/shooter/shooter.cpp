#include "shooter.hpp"
#include "p2p/p2p.hpp"
#include "app/app.hpp"
namespace Shooter {
    Shooter shooter;

    namespace {
        float clamp(float t, float a, float b) {
            return t > b 
                ? b 
                : t < a
                ? a
                : t;
        }
    }

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
        constexpr int turn_scale = 500;
        constexpr int accel_scale = 500;
        constexpr int drag_max = 100;
        constexpr int vel_max = 300;

        ship.is_boosting = false;
        Vec2 dir = ship.dir();
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[left_code]) {
            ship.deg -= turn_scale * dt;
        }
        if (state[right_code]) {
            ship.deg += turn_scale * dt;
        }
        if (state[up_code]) {
            ship.vel.y += dir.y * dt * accel_scale;
            ship.vel.x += dir.x * dt * accel_scale;
            ship.is_boosting = true;
        }
        if (ship.vel.l2() >= vel_max * vel_max) {
            ship.vel.normalise(vel_max);
        }
        ship.pos.x += ship.vel.x * dt;
        ship.pos.y += ship.vel.y * dt;
        if (!ship.is_boosting) {
            ship.vel.x -= clamp(ship.vel.x, -drag_max, drag_max) * dt;
            ship.vel.y -= clamp(ship.vel.y, -drag_max, drag_max) * dt;
        }

        while (ship.pos.x < 0) ship.pos.x += width;
        while (ship.pos.y < 0) ship.pos.y += height;
        while (ship.pos.x > width) ship.pos.x -= width;
        while (ship.pos.y > height) ship.pos.y -= height;
    }

    void Shooter::step() {
        float dt = App::app.frame_stopwatch.delta() / 1000.0f;
        step_ship(ship1, dt, SDL_SCANCODE_W, SDL_SCANCODE_A, SDL_SCANCODE_D);
        step_ship(ship2, dt, SDL_SCANCODE_UP, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
        bool ship1_hit = false;
        bool ship2_hit = false;
        for (int i = 0; i < projectiles_size; i++) {
            step_projectile(projectiles[i], dt);
            ship1_hit |= ship1.is_hit(projectiles[i]);
            ship2_hit |= ship2.is_hit(projectiles[i]);
        }
        if (ship1_hit && ship2_hit) {
            Chat::chat.messages.push_back("Draw!");
        } else if (ship1_hit) {
            Chat::chat.messages.push_back("Ship 2 Won!");
            ship2_score++;
        } else if (ship2_hit) {
            Chat::chat.messages.push_back("Ship 1 Won!");
            ship1_score++;
        } else {
            return;
        }
        Chat::chat.messages.push_back("Score = " + std::to_string(ship1_score) + " : " + std::to_string(ship2_score));
        App::app.reset_to_state(App::AppState_GameSelect);
    }
}