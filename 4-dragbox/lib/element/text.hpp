#ifndef LIB_ELEMENT_TEXT_HPP
#define LIB_ELEMENT_TEXT_HPP

#include "element.hpp"
#include "../actions/actions.hpp"
#include <SDL.h>
#include <SDL_ttf.h>

struct Text: Element {
    TTF_Font* font;
    const char* text;
    SDL_Color color;
    SDL_Color hl_color;

    Text(
        const char* text, 
        TTF_Font* font,
        float angle = 0,
        SDL_Color color = {0, 0, 0, 255},
        SDL_Color hl_color = {0, 0, 255, 255}
    ): text(text), font(font), color(color), hl_color(hl_color) {
        this->angle = angle;
    }

    bool reload_text(TTF_Font* font, const char* text, SDL_Color& color);

    virtual bool on_mount() override;
    virtual bool draw() override;
    virtual bool step() override;
    virtual bool handle_action(const Action&) override;
};

#endif