#include "circle.hpp"
#include "../app.hpp"
#include "../utils.hpp"

void Circle::reload_tex(const SDL_Color& color) {
    SDL_DestroyTexture(tex);
    tex = SDL_CreateTexture(
        app->renderer, 
        SDL_PIXELFORMAT_ABGR8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        rect.w, 
        rect.h
    );
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    u_int32_t _format;
    SDL_QueryTexture(tex, &_format, NULL, NULL, NULL);
    SDL_PixelFormat* format = SDL_AllocFormat(_format);
    u_int32_t bcolor = SDL_MapRGBA(format, color.r, color.g, color.b, color.a);
    SDL_FreeFormat(format);

    u_int32_t* pixels;
    int pitch;
    SDL_LockTexture(tex, NULL, reinterpret_cast<void**>(&pixels), &pitch);
    float rr = rect.w / 2.0;
    float cr = rect.h / 2.0;
    for (int r = 0; r < rect.w; r++) {
        uint32_t* rowpix = unsafe_shift(pixels, r * pitch);
        for (int c = 0; c < rect.h; c++) {
            float dr = (r - rr) / rr;
            float dc = (c - cr) / cr;
            float d2 = dr * dr + dc * dc;
            if (d2 < 1.0) {
                rowpix[c] = bcolor;
            }
        }
    }
    SDL_UnlockTexture(tex);
}

bool Circle::on_mount()  {
    reload_tex(color);
    return true;
}

bool Circle::draw() {
    SDL_RenderCopyF(
        app->renderer,
        tex,
        NULL, &rect
    );
    return true;
}

bool Circle::handle_action(const Action& action) {
    switch(action.type) {
    case HIGHLIGHT_TYPE:
        {
            if (action.highlight.highlight) {
                reload_tex(hl_color);
            } else {
                reload_tex(color);
            }
        }
        break;
    }
    return true;
}
