#include "text.hpp"
#include "../app.hpp"

bool Text::reload_text(TTF_Font* font, const char* text, SDL_Color& color) {
    SDL_DestroyTexture(tex);
    SDL_Surface* text_surface;
    text_surface = TTF_RenderText_Solid(font, text, color); 
    tex = SDL_CreateTextureFromSurface(app->renderer, text_surface);
    // {
    //     u_int32_t format;
    //     SDL_QueryTexture(tex, &format, NULL, NULL, NULL);
    //     std::cout << SDL_GetPixelFormatName(format) << "\n"; // SDL_PIXELFORMAT_ARGB8888
    // }
    SDL_FreeSurface(text_surface);
    return true;
}

bool Text::on_mount() {
    return reload_text(font, text, color);
}

bool Text::draw() {
    SDL_FPoint center{rect.w / 2, rect.h / 2};
    SDL_RenderCopyExF(
        app->renderer, 
        tex, 
        NULL, &rect, 
        angle, &center,
        SDL_FLIP_NONE
    );
    return true;
}

bool Text::step() {
    angle += 5;
    return true;
}

bool Text::handle_action(const Action& action) {
    switch(action.type) {
    case HIGHLIGHT_TYPE:
        {
            if (action.highlight.highlight) {
                reload_text(font, text, hl_color);
            } else {
                reload_text(font, text, color);
            }
        }
        break;
    }
    return true;
}
