#ifndef LIB_CONNECT4_HPP
#define LIB_CONNECT4_HPP

#include <iostream>
#include "viewport.hpp"
#include "utils.hpp"

struct Board;

struct Color {
    int r; 
    int g; 
    int b;
};

struct Cell: public ViewPort<Cell> {

    bool post_init(SDL_Renderer* renderer, int w, int h) {
        set_color({255, 0, 0});
    }

    bool set_color(Color color) {
        uint32_t* pix;
        int pitch;
        SDL_LockTexture(tex, NULL, reinterpret_cast<void**>(&pix), &pitch);
        float midc = static_cast<float>(pos.w) / 2;
        float midr = static_cast<float>(pos.h) / 2;
        for (int r = 0; r < pos.h; r++) {
            uint32_t* rowpix = unsafe_shift(pix, r * pitch);
            for (int c = 0; c < pos.w; c++) {
                float dx = static_cast<float>(c - midc) / midc;
                float dy = static_cast<float>(r - midr) / midr;
                if (dx * dx + dy * dy <= 1.0) {
                    rowpix[c] = SDL_MapRGBA(format, color.r, color.g, color.b, 255);
                }
            }
        }
        SDL_UnlockTexture(tex);
    }
};

#endif