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
        if (!projectile.step(dt)) return;
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
        ship.step(dt, up_code, left_code, right_code);
        while (ship.pos.x < 0) ship.pos.x += width;
        while (ship.pos.y < 0) ship.pos.y += height;
        while (ship.pos.x > width) ship.pos.x -= width;
        while (ship.pos.y > height) ship.pos.y -= height;
    }

    void Shooter::step() {
        float dt = App::app.frame_stopwatch.delta() / 1000.0f;
        step_ship(ship1, dt, SDL_SCANCODE_W, SDL_SCANCODE_A, SDL_SCANCODE_D);
        step_ship(ship2, dt, SDL_SCANCODE_UP, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
        bool ship1_shot_down = false;
        bool ship2_shot_down = false;
        for (int i = 0; i < projectiles_size; i++) {
            step_projectile(projectiles[i], dt);
            ship1_shot_down |= ship1.is_shot_down(projectiles[i]);
            ship2_shot_down |= ship2.is_shot_down(projectiles[i]);
        }
        if (ship1_shot_down && ship2_shot_down) {
            Chat::chat.messages.push_back("Draw!");
        } else if (ship1_shot_down) {
            Chat::chat.messages.push_back("Ship 2 Won!");
            ship2_score++;
        } else if (ship2_shot_down) {
            Chat::chat.messages.push_back("Ship 1 Won!");
            ship1_score++;
        } else {
            return;
        }
        Chat::chat.messages.push_back("Score = " + std::to_string(ship1_score) + " : " + std::to_string(ship2_score));
        App::app.reset_to_state(App::AppState_GameSelect);
    }
}