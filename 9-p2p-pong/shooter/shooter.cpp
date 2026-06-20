#include "shooter.hpp"
#include "p2p/p2p.hpp"

namespace Shooter {
    Shooter shooter;

    void Shooter::process_takedown() {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W]) {
            ship1.pos.y += -10.0;
        }
        if (state[SDL_SCANCODE_S]) {
            ship1.pos.y += 10.0;
        }
        if (state[SDL_SCANCODE_A]) {
            ship1.pos.x += -10.0;
        }
        if (state[SDL_SCANCODE_D]) {
            ship1.pos.x += 10.0;
        }
    }
}