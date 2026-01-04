#include "element.hpp"
#include "app.hpp"

// Element 
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
        hovered = true;
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


// Text

void Text::reload_text(TTF_Font* font, const char* text, SDL_Color& color) {
    SDL_DestroyTexture(tex);
    SDL_Surface* text_surface;
    text_surface = TTF_RenderText_Solid(font, text, color);
    tex = SDL_CreateTextureFromSurface(app->renderer, text_surface);
    SDL_FreeSurface(text_surface);
}

void Text::on_mount() {
    reload_text(font, text, color);
}

void Text::draw() {
    SDL_RenderCopy(app->renderer, tex, NULL, &rect);
}

void Text::handle_unhover(const SDL_Event&) {
    reload_text(font, text, color);
}

void Text::handle_mouse_down(const SDL_MouseButtonEvent& e) {
    reload_text(font, text, hl_color);
}

void Text::handle_mouse_up(const SDL_MouseButtonEvent& e) {
    reload_text(font, text, color);
}

// DragBox

void DragBox::handle_mouse_motion(const SDL_MouseMotionEvent& e) {
    if (!app->click_down || last_hover == nullptr) return;
    last_hover->update_rect(e.x - focus_off.x, e.y - focus_off.y);
}

void DragBox::handle_mouse_down(const SDL_MouseButtonEvent& e) {
    if (last_hover == nullptr) return;
    focus_off.x = app->mouse_pos.x - last_hover->rect.x;
    focus_off.y = app->mouse_pos.y - last_hover->rect.y;
}
