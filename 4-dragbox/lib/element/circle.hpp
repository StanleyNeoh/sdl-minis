#ifndef LIB_ELEMENT_CIRCLE_HPP
#define LIB_ELEMENT_CIRCLE_HPP

#include "element.hpp"
#include "../actions/actions.hpp"
#include <SDL.h>

struct Circle: Element {
    int r;
    SDL_Color color;
    SDL_Color hl_color;

    // Managed
    Circle(int r, SDL_Color color, SDL_Color hl_color): r(r), color(color), hl_color(hl_color) {}

    void reload_tex(const SDL_Color& color);
    virtual bool on_mount() override;
    virtual bool draw() override;
    virtual bool handle_action(const Action&) override;
};

#endif