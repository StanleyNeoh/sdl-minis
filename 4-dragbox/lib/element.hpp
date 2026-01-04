#ifndef LIB_ELEMENT_HPP
#define LIB_ELEMENT_HPP

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>

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
    SDL_Rect rect;

    App* app = nullptr;
    std::vector<Frame> uninitalized;
    std::vector<Element*> children;
    Element* last_hover = nullptr;
    bool hovered = false;

    Element() = default;
    virtual ~Element() = default;

    void add_child(Element* elem, float rel_x, float rel_y, float rel_w, float rel_h) {
        uninitalized.push_back({
            elem,
            rel_x,
            rel_y,
            rel_w,
            rel_h
        });
    }

    void init(App& app, float x, float y, float w, float h) {
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
            e.elem->init(app, _x, _y, _w, _h);
            children.push_back(e.elem);
        }
        uninitalized.clear();
        on_mount();
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

    void draw_all() {
        draw();
        for (Element* e: children) {
            e->draw();
        }
    }

    void update_rect(int x, int y, int w = -1, int h = -1) {
        rect.x = x;
        rect.y = y;
        if (w >= 0) rect.w = w;
        if (h >= 0) rect.h = h;
    }

    virtual void on_mount() {}
    virtual void draw() {}
    virtual void handle_unhover(const SDL_Event&){}
    virtual void handle_quit(const SDL_QuitEvent&) {}
    virtual void handle_mouse_scroll(const SDL_MouseWheelEvent&) {}
    virtual void handle_mouse_motion(const SDL_MouseMotionEvent&) {}
    virtual void handle_mouse_down(const SDL_MouseButtonEvent&) {}
    virtual void handle_mouse_up(const SDL_MouseButtonEvent&) {}
    virtual void handle_key_down(const SDL_KeyboardEvent&) {}
    virtual void handle_key_up(const SDL_KeyboardEvent&) {}
};

struct Text: Element {
    TTF_Font* font;
    const char* text;
    SDL_Color color;
    SDL_Color hl_color;

    // Managed
    SDL_Texture* tex = NULL;

    Text(
        const char* text, 
        TTF_Font* font,
        SDL_Color color = {0, 0, 0, 255},
        SDL_Color hl_color = {0, 0, 255, 255}
    ): text(text), font(font), color(color), hl_color(hl_color) {}

    virtual ~Text() override {
        if (tex != NULL) {
            SDL_DestroyTexture(tex);
        }
    }

    void reload_text(TTF_Font* font, const char* text, SDL_Color& color);

    virtual void on_mount() override;
    virtual void draw() override;
    virtual void handle_unhover(const SDL_Event&) override;
    virtual void handle_mouse_down(const SDL_MouseButtonEvent&) override;
    virtual void handle_mouse_up(const SDL_MouseButtonEvent&) override;
};

struct DragBox: Element {
    // State
    SDL_Point focus_off = {-1, -1};

    DragBox() = default;
    virtual ~DragBox() = default;

    virtual void handle_mouse_motion(const SDL_MouseMotionEvent& e) override;
    virtual void handle_mouse_down(const SDL_MouseButtonEvent& e) override;
};

#endif