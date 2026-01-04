#include "dragbox.hpp"
#include "../app.hpp"

void DragBox::handle_mouse_motion(const SDL_MouseMotionEvent& e) {
    if (focus == nullptr) return;
    focus->update_pos(e.x - focus_off.x, e.y - focus_off.y);
}

void DragBox::handle_mouse_down(const SDL_MouseButtonEvent& e) {
    if (last_hover == nullptr) return;
    focus = last_hover;
    focus_off.x = app->mouse_pos.x - last_hover->rect.x;
    focus_off.y = app->mouse_pos.y - last_hover->rect.y;
    focus->handle_action(Action{highlight: HighlightAction(true)});
}

void DragBox::handle_mouse_up(const SDL_MouseButtonEvent& e) {
    if (focus != nullptr) {
        focus->handle_action(Action{highlight: HighlightAction(false)});
        focus = nullptr;
    }
}