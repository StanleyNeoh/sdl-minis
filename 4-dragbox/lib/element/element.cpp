#include "element.hpp"
#include "../app.hpp"

void Element::handle_event(const SDL_Event& event) {
    // Event comes in iff 
    // - hovered 
    // - transitioning from hovered to unhovered

    // Handle child first
    last_hover = nullptr;
    for (Element* e: children) {
        if (e->is_overlap(app->mouse_pos)) {
            last_hover = e;
        } else if (e->hovered) {
            e->handle_event(event);
        }
    }
    if (last_hover != nullptr) {
        last_hover->handle_event(event);
    }

    // Hendle self
    if (hovered && !is_overlap(app->mouse_pos)) {
        handle_unhover(event);
        hovered = false;
    } else {
        if (!hovered) {
            handle_hover(event);
            hovered = true;
        }
        switch(event.type) {
        case SDL_QUIT:
            handle_quit(event.quit);
            break;
        case SDL_MOUSEWHEEL:
            handle_mouse_scroll(event.wheel);
            break;
        case SDL_MOUSEMOTION:
            handle_mouse_motion(event.motion);
            break;
        case SDL_MOUSEBUTTONDOWN:
            handle_mouse_down(event.button);
            break;
        case SDL_MOUSEBUTTONUP:
            handle_mouse_up(event.button);
            break;
        case SDL_KEYDOWN:
            handle_key_down(event.key);
            break;
        case SDL_KEYUP:
            handle_key_up(event.key);
            break;
        default:
            break;
        }
    }
}

