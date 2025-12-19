#ifndef LIB_VIEWPORT_HPP
#define LIB_VIEWPORT_HPP

#include <SDL.h>

template <typename T>
struct ViewPort {
    SDL_Texture* tex;
    SDL_PixelFormat* format;
    SDL_Rect rect;

    ViewPort(SDL_Rect pos): rect(pos), tex(NULL), format(NULL) {};
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

    ViewPort(const ViewPort<T>& other) = delete;
    ViewPort(ViewPort<T>&& other): tex(other.tex), format(other.format), rect(other.rect) {
        other.tex = NULL;
        other.format = NULL;
    }

    ViewPort<T>& operator=(const ViewPort<T>& other) = delete;
    ViewPort<T>& operator=(ViewPort<T>&& other) noexcept {
        ViewPort<T> temp = std::move(other);
        std::swap(*this, temp);
        return *this;
    }

    bool init_tex(SDL_Renderer* renderer, int x, int y, int w, int h) {
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (tex == NULL) {
            std::cerr << "[Viewport::init] Error: " << SDL_GetError() << "\n";
            return false;
        }
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
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
        return true;
    }

    bool init(SDL_Renderer* renderer, int x, int y, int w, int h) {
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
        return true;
    }

    bool draw_tex(SDL_Renderer* renderer, SDL_Texture* tex) {
        if (SDL_RenderCopy(renderer, tex, NULL, &rect) != 0) {
            std::cerr << "[Viewport::draw] Error: " << SDL_GetError() << "\n";
            return false;
        }
        return true;
    }

    bool draw(SDL_Renderer* renderer) { return true; }

    bool step() { return false; }

    bool handle_event(SDL_Event& e, bool& quit) {
        T* self = static_cast<T*>(this);
        switch(e.type) {
            case SDL_QUIT:
                quit = true;
                self->handle_quit(e.quit);
                return true;
            case SDL_MOUSEWHEEL:
                self->handle_mouse_scroll(e.wheel);
                return true;
            case SDL_MOUSEMOTION:
                {
                    SDL_MouseMotionEvent me = e.motion;
                    me.x -= rect.x;
                    me.y -= rect.y;
                    if (me.x < 0 || me.x >= rect.w) return true;
                    if (me.y < 0 || me.y >= rect.h) return true;
                    self->handle_mouse_motion(me);
                }
                return true;
            case SDL_MOUSEBUTTONDOWN:
                self->handle_mouse_down(e.button);
                return true;
            case SDL_MOUSEBUTTONUP:
                self->handle_mouse_up(e.button);
                return true;
            case SDL_KEYDOWN:
                self->handle_key_down(e.key);
                return true;
            case SDL_KEYUP:
                self->handle_key_up(e.key);
                return true;
            default:
                return false;
        }
    }

    void handle_quit(const SDL_QuitEvent&) {}
    void handle_mouse_scroll(const SDL_MouseWheelEvent&) {}
    void handle_mouse_motion(const SDL_MouseMotionEvent&) {}
    void handle_mouse_down(const SDL_MouseButtonEvent&) {}
    void handle_mouse_up(const SDL_MouseButtonEvent&) {}
    void handle_key_down(const SDL_KeyboardEvent&) {}
    void handle_key_up(const SDL_KeyboardEvent&) {}
};

#endif