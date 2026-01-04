#ifndef LIB_ELEMENT_DRAGBOX
#define LIB_ELEMENT_DRAGBOX

#include "element.hpp"
#include <SDL.h>

struct DragBox: Element {
    // State
    Element* focus = nullptr;
    SDL_Point focus_off = {-1, -1};

    DragBox() = default;
    virtual ~DragBox() = default;

    virtual void handle_mouse_motion(const SDL_MouseMotionEvent& e) override;
    virtual void handle_mouse_down(const SDL_MouseButtonEvent& e) override;
    virtual void handle_mouse_up(const SDL_MouseButtonEvent& e) override;
};


#endif