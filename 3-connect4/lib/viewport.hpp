#ifndef LIB_VIEWPORT_HPP
#define LIB_VIEWPORT_HPP

#include <SDL.h>

struct Pad {
    int padL;
    int padR;
    int padT;
    int padB;
};

template <typename T>
struct ViewPort {
    SDL_Texture* tex;
    SDL_PixelFormat* format;
    SDL_Rect pos;

    ViewPort(SDL_Rect pos): pos(pos), tex(NULL), format(NULL) {};
    ViewPort(): ViewPort({0, 0, 0, 0}) {};

    ~ViewPort() {
        if (tex != NULL) {
            SDL_DestroyTexture(tex);
            tex = NULL;
        }
        if (format != NULL) {
            SDL_FreeFormat(format);
            format = NULL;
        }
    }

    bool init(SDL_Renderer* renderer, int w, int h) {
        pos.w = w;
        pos.h = h;
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (tex == NULL) {
            std::cerr << "[Viewport::init] Error: " << SDL_GetError() << "\n";
            return false;
        }
        uint32_t formatEnum;
        if (SDL_QueryTexture(tex, &formatEnum, NULL, NULL, NULL) != 0) {
            std::cerr << "[Viewport::init] Error: " << SDL_GetError() << "\n";
            return false;
        }
        format = SDL_AllocFormat(formatEnum);
        if (format == NULL) {
            std::cerr << "[Viewport::init] Error: " << SDL_GetError() << "\n";
            return false;
        }
        static_cast<T*>(this)->post_init(renderer, w, h);
        return true;
    }

    bool post_init(SDL_Renderer* renderer, int w, int h) {}

    bool draw(SDL_Renderer* renderer) {
        if (SDL_RenderCopy(renderer, tex, NULL, &pos) != 0) {
            std::cerr << "[Viewport::draw] Error: " << SDL_GetError() << "\n";
            return false;
        }
        return true;
    }
};

#endif