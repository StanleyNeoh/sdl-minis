#ifndef LIB_CONTROL_HPP
#define LIB_CONTROL_HPP

#include "viewport.hpp"
#include "sketchpad.hpp"
#include "utils.hpp"

struct Button: public ViewPort<Button> {
    int padding = 5;
    bool hover = false;;
    SketchPad& sketchpad;
    Color color;

    Button(SketchPad& sketchpad, int r, int g, int b):
        sketchpad(sketchpad),
        color{
            clamp(r, 0, 255),
            clamp(g, 0, 255),
            clamp(b, 0, 255)
        }
    {}

    void handle_mouse_motion(SDL_Surface* surface, const SDL_MouseMotionEvent& e) {
        if (e.x < 0 || e.y < 0 || e.x > w || e.y > h) {
            hover = false;
        } else {
            hover = true;
        }
    }

    void handle_mouse_down(SDL_Surface* surface, const SDL_MouseButtonEvent& e) {
        if (!hover) return;
        std::cout << color.r << color.g << color.b << " click" << std::endl;
        sketchpad.setColor(color);
    }

    bool draw_surface(SDL_Surface* surface) {
        SDL_Rect all{x, y, w, h};
        if (hover) {
            SDL_FillRect(surface, &all, SDL_MapRGB(surface->format, 255, 255, 255));
        } else {
            SDL_FillRect(surface, &all, SDL_MapRGB(surface->format, 0, 0, 0));
        }
        SDL_Rect inner{x+padding, y+padding, w-2*padding, h-2*padding};
        SDL_FillRect(surface, &inner, SDL_MapRGB(surface->format, color.r, color.g, color.b));
        return true;
    }
};

#endif