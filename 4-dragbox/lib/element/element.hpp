#ifndef LIB_ELEMENT_ELEMENT_HPP
#define LIB_ELEMENT_ELEMENT_HPP

#include <SDL.h>
#include <iostream>
#include <vector>
#include "math.h"
#include "../actions/actions.hpp"

struct App;
struct Element;

struct Frame {
    Element* elem;
    float rel_x;
    float rel_y;
    float rel_w;
    float rel_h;
};

struct Element {
    // Coordinates of element from global
    SDL_FRect rect;
    float angle = 0.0;

    App* app = nullptr;
    std::vector<Frame> uninitalized;
    std::vector<Element*> children;
    Element* last_hover = nullptr;
    bool hovered = false;

    SDL_Texture* tex = NULL;
    SDL_PixelFormat* format = NULL;

    Element() = default;
    virtual ~Element() {
        if (tex != NULL) SDL_DestroyTexture(tex);
        if (format != NULL) SDL_FreeFormat(format);
    };

    void add_child(Element* elem, float rel_x, float rel_y, float rel_w, float rel_h) {
        uninitalized.push_back({
            elem,
            rel_x,
            rel_y,
            rel_w,
            rel_h
        });
    }

    bool init(App& app, float x, float y, float w, float h) {
        this->app = &app;
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
        for (auto& e: uninitalized) {
            float _x = e.rel_x * w + x;
            float _y = e.rel_y * h + y;
            float _w = e.rel_w * w;
            float _h = e.rel_h * h;
            if (!e.elem->init(app, _x, _y, _w, _h)) {
                return false;
            }
            children.push_back(e.elem);
        }
        uninitalized.clear();
        return on_mount();
    }

    bool is_overlap(const SDL_Point& global_p) const {
        return (
            global_p.x >= rect.x &&
            global_p.y >= rect.y &&
            global_p.x < rect.x + rect.w &&
            global_p.y < rect.y + rect.h
        );
    }

    void handle_event(const SDL_Event& event);

    bool draw_all() {
        if (!draw()) return false;
        for (Element* e: children) {
            if (!e->draw()) return false;
        }
        return true;
    }

    void translate(float dx, float dy) {
        rect.x += dx;
        rect.y += dy;
        for (Element* e: children) {
            e->translate(dx, dy);
        }
    }

    void rotate(float deg) {
        angle = fmod(angle + deg, 360);
        for (Element* e: children) {
            e->rotate(deg);
        }
    }

    void update_pos(float x, float y) {
        translate(x - rect.x, y - rect.y);
    }

    void update_rot(float angle) {
        rotate(angle - this->angle);
    }

    bool step_all() {
        if (!step()) return false;
        for (Element* e: children) {
            if (!e->step_all()) return false;
        }
        return true;
    }

    virtual bool handle_action(const Action& action) { return true; }
    virtual bool on_mount() { return true; }
    virtual bool step() { return true; }
    virtual bool draw() { return true; }
    virtual void handle_hover(const SDL_Event&){}
    virtual void handle_unhover(const SDL_Event&){}
    virtual void handle_quit(const SDL_QuitEvent&) {}
    virtual void handle_mouse_scroll(const SDL_MouseWheelEvent&) {}
    virtual void handle_mouse_motion(const SDL_MouseMotionEvent&) {}
    virtual void handle_mouse_down(const SDL_MouseButtonEvent&) {}
    virtual void handle_mouse_up(const SDL_MouseButtonEvent&) {}
    virtual void handle_key_down(const SDL_KeyboardEvent&) {}
    virtual void handle_key_up(const SDL_KeyboardEvent&) {}
};


#endif