#ifndef LIB_VIEWPORT_HPP
#define LIB_VIEWPORT_HPP

#include <SDL2/SDL.h>
#include <array>
#include <tuple>
#include <type_traits>
#include "utils.hpp"

template <typename T>
struct Frame {
    T& viewport;
    int scale;

    Frame(T& viewport, int scale): viewport(viewport), scale(scale) {}
};

template <typename T>
struct ViewPort {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    // CRTP dispatch
    bool handle_event(SDL_Surface* surface, SDL_Event& e, bool& quit) {
        T* self = static_cast<T*>(this);
        switch(e.type) {
            case SDL_QUIT:
                quit = true;
                self->handle_quit(surface, e.quit);
                return true;
            case SDL_MOUSEWHEEL:
                self->handle_mouse_scroll(surface, e.wheel);
                return true;
            case SDL_MOUSEMOTION:
                {
                    SDL_MouseMotionEvent me = e.motion;
                    me.x -= x;
                    me.y -= y;
                    self->handle_mouse_motion(surface, me);
                }
                return true;
            case SDL_MOUSEBUTTONDOWN:
                self->handle_mouse_down(surface, e.button);
                return true;
            case SDL_MOUSEBUTTONUP:
                self->handle_mouse_up(surface, e.button);
                return true;
            case SDL_KEYDOWN:
                self->handle_key_down(surface, e.key);
                return true;
            case SDL_KEYUP:
                self->handle_key_up(surface, e.key);
                return true;
            default:
                return false;
        }
    }

    bool draw_surface(SDL_Surface* surface) {
        if (SDL_LockSurface(surface) != 0) return false;
        T* self = static_cast<T*>(this);
        uint32_t* pixels = reinterpret_cast<uint32_t*>(surface->pixels);
        pixels = unsafe_shift(pixels, y * surface->pitch) + x;
        bool success = self->draw(pixels, surface->pitch, surface->format);
        SDL_UnlockSurface(surface);
        return success;
    }

    void update_layout(int w, int h) {
        this->w = w;
        this->h = h;
    }
   
    // Default implementations
    void handle_quit(SDL_Surface*, const SDL_QuitEvent&) {}
    void handle_mouse_scroll(SDL_Surface*, const SDL_MouseWheelEvent&) {}
    void handle_mouse_motion(SDL_Surface*, const SDL_MouseMotionEvent&) {}
    void handle_mouse_down(SDL_Surface*, const SDL_MouseButtonEvent&) {}
    void handle_mouse_up(SDL_Surface*, const SDL_MouseButtonEvent&) {}
    void handle_key_down(SDL_Surface*, const SDL_KeyboardEvent&) {}
    void handle_key_up(SDL_Surface*, const SDL_KeyboardEvent&) {}
    bool draw(uint32_t* pixels, int pitch, SDL_PixelFormat* format) { return true; }
};

template <typename U, typename... Ts>
struct GenericViewPortContainer: ViewPort<U> {
    static constexpr int n_items = sizeof...(Ts);
    std::tuple<Frame<Ts>...> frames;

    GenericViewPortContainer(Frame<Ts>... args) : frames(std::make_tuple(args...)) {}

    template <std::size_t I = 0>
    typename std::enable_if<I == n_items, int>::type
    sum_scale() {
        return 0;
    }

    template <std::size_t I = 0>
    typename std::enable_if<I < n_items, int>::type
    sum_scale() {
        return std::get<I>(frames).scale + sum_scale<I+1>();
    }

    bool handle_event(SDL_Surface* surface, SDL_Event& e, bool& quit) {
        bool handled = false;
        tuple_for_each(frames, [&](auto& frame) {
            if (frame.viewport.handle_event(surface, e, quit)) handled = true;
        });
        return handled;
    }

    bool draw_surface(SDL_Surface* surface) {
        bool drawn = false;
        tuple_for_each(frames, [&](auto& frame) {
            if (frame.viewport.draw_surface(surface)) drawn = true;
        });
        return drawn;
    }
};

template <typename... Ts>
struct ViewPortRow: public GenericViewPortContainer<ViewPortRow<Ts...>, Ts...> {
    using Base = GenericViewPortContainer<ViewPortRow<Ts...>, Ts...>;
    using Base::n_items;
    using Base::frames;

    ViewPortRow(Frame<Ts>... args) : Base(args...) {}

    template <std::size_t I = 0>
    typename std::enable_if_t<I == n_items, void>
    set_children(int curr_x, int total_scale) {}

    template <std::size_t I = 0>
    typename std::enable_if_t<I < n_items, void>
    set_children(int curr_x, int total_scale) {
        auto& frame = std::get<I>(frames);
        int item_width = (this->w * frame.scale) / total_scale;
        
        frame.viewport.x = curr_x;
        frame.viewport.y = this->y;
        frame.viewport.update_layout(item_width, this->h);
        
        set_children<I+1>(curr_x + item_width, total_scale);
    }

    void update_layout(int w, int h) {
        this->w = w;
        this->h = h;
        int total = this->sum_scale();
        if (total > 0) set_children<0>(this->x, total);
    }
};

template <typename... Ts>
struct ViewPortCol: public GenericViewPortContainer<ViewPortCol<Ts...>, Ts...> {
    using Base = GenericViewPortContainer<ViewPortCol<Ts...>, Ts...>;
    using Base::n_items;
    using Base::frames;

    ViewPortCol(Frame<Ts>... args) : Base(args...) {}

    template <std::size_t I = 0>
    typename std::enable_if_t<I == n_items, void>
    set_children(int curr_y, int total_scale) {}

    template <std::size_t I = 0>
    typename std::enable_if_t<I < n_items, void>
    set_children(int curr_y, int total_scale) {
        auto& frame = std::get<I>(frames);
        int item_height = (this->h * frame.scale) / total_scale;
        
        frame.viewport.x = this->x;
        frame.viewport.y = curr_y;
        frame.viewport.update_layout(this->w, item_height);

        set_children<I+1>(curr_y + item_height, total_scale);
    }

    void update_layout(int w, int h) {
        this->w = w;
        this->h = h;
        int total = this->sum_scale();
        if (total > 0) set_children<0>(this->y, total);
    }
};

#endif