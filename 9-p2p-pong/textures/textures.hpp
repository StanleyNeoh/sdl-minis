#ifndef TEXTURES_TEXTURES_HPP
#define TEXTURES_TEXTURES_HPP

#include "SDL.h"

namespace Textures {
    struct Textures {
        SDL_Renderer* renderer = nullptr;

        bool initialise(SDL_Renderer* _renderer) {
            if (renderer != nullptr) return false;
            renderer = _renderer;
            return true;
        }

        SDL_Texture* get_ship_tex();
        SDL_Texture* get_trail_fire_tex();
    };

    extern Textures textures;
}


#endif