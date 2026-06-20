#include "shooter.hpp"
#include "p2p/p2p.hpp"
#include "app/app.hpp"

namespace Shooter {
    Shooter shooter;

    void Shooter::step() {
        float dt = App::app.frame_stopwatch.delta() / 1000.0f;
        float scale = 300.0 * dt;
        float rad = ship1.deg * (M_PI / 180.0f);
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_A]) {
            ship1.deg -= scale;
        }
        if (state[SDL_SCANCODE_D]) {
            ship1.deg += scale;
        }
        if (state[SDL_SCANCODE_W]) {
            ship1.vel.y += -cos(rad) * scale;
            ship1.vel.x += sin(rad) * scale;
        }
        if (state[SDL_SCANCODE_S]) {
            ship1.vel.y -= -cos(rad) * scale;
            ship1.vel.x -= sin(rad) * scale;
        }
        if (ship1.vel.l2() >= 300.0 * 300.0) {
            ship1.vel.normalise(300.0);
        }
        ship1.pos.x += ship1.vel.x * dt;
        ship1.pos.y += ship1.vel.y * dt;

        while (ship1.pos.x < 0) ship1.pos.x += width;
        while (ship1.pos.y < 0) ship1.pos.y += height;
        while (ship1.pos.x > width) ship1.pos.x -= width;
        while (ship1.pos.y > height ) ship1.pos.y -= height;
        std::cout << ship1.pos << ship1.vel << "\n";
    }
}