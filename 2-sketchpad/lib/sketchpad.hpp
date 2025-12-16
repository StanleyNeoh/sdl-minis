#ifndef LIB_SKETCHPAD_HPP
#define LIB_SKETCHPAD_HPP

#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <cmath>

#include "utils.hpp"
#include "viewport.hpp"

struct Color {
    int r;
    int g;
    int b;
};

struct SketchPad: public ViewPort<SketchPad> {
    // Params
    int borderRadius = 1;

    // State
    SDL_Point mouse_pos{-1, -1};
    int radius = 20;
    bool mouse_down = false;
    Color color{255, 255, 255};

    std::vector<uint32_t> canvas;

    void setColor(const Color& color) {
        this->color = color;
    }

    void handle_quit(SDL_Surface* surface, const SDL_QuitEvent& e) {}

    void update_layout(int w, int h) {
        this->w = w;
        this->h = h;
        canvas.resize(w * h, 0);
    }

    void handle_mouse_scroll(SDL_Surface* surface, const SDL_MouseWheelEvent& e) {
        if (mouse_pos.x < 0) return;
        radius = clamp(radius + e.y, 0, 100);
    }

    void handle_mouse_motion(SDL_Surface* surface, const SDL_MouseMotionEvent& e) {
        if (e.x < 0 || e.y < 0 || e.x > w || e.y > h) {
            mouse_pos.x = -1;
            mouse_pos.y = -1;
            return;
        } else {
            if (mouse_down) {
                draw_line(
                    canvas.data(),
                    w,
                    h,
                    mouse_pos,
                    {e.x, e.y},
                    radius,
                    SDL_MapRGBA(surface->format, color.r, color.g, color.b, 255)
                );
            }
            mouse_pos.x = e.x;
            mouse_pos.y = e.y;
        }
    }

    void handle_mouse_down(SDL_Surface* surface, const SDL_MouseButtonEvent& e) {
        if (mouse_pos.x < 0) return;
        mouse_down = true;
        draw_line(
            canvas.data(),
            w,
            h,
            mouse_pos,
            mouse_pos,
            radius,
            SDL_MapRGBA(surface->format, color.r, color.g, color.b, 255)
        );
    }

    void handle_mouse_up(SDL_Surface* surface, const SDL_MouseButtonEvent& e) {
        mouse_down = false;
    }

    void handle_key_down(SDL_Surface* surface, const SDL_KeyboardEvent& e) {
    }
    
    void handle_key_up(SDL_Surface* surface, const SDL_KeyboardEvent& e) {

    }

    bool draw(uint32_t* pixels, int pitch, SDL_PixelFormat* format) {
        blit_canvas(pixels, pitch, format);
        blit_cursor(pixels, pitch, format);
        return true;
    }

private:
    void draw_circle(
        uint32_t* dst, int w, int h, 
        SDL_Point p, int r, uint32_t color, 
        int br = -1, int dstPitch = -1
    ) {
        if (p.x < 0) return;
        if (dstPitch < 0) dstPitch = w * sizeof(uint32_t);

        SDL_Point top_left, bottom_right;
        get_bounds<1>({p}, top_left, bottom_right, r + br, w, h);

        bool once = false;
        for (int y = top_left.y; y <= bottom_right.y; y++) {
            uint32_t* row_pixels = unsafe_shift(dst, y * dstPitch);
            for (int x = top_left.x; x <= bottom_right.x; x++) {
                int d2 = dist2(p, {x, y});
                int r = std::sqrt(d2);
                if (
                    (br < 0 && r < radius) ||
                    (br >= 0 && std::abs(r - radius) < br)
                ) {
                    row_pixels[x] = color;
                }
            }
        }
    }

    void draw_line(
        uint32_t* dst, int w, int h, 
        SDL_Point p0, SDL_Point p1, int r, uint32_t color
    ) {
        SDL_Point tl, br;
        get_bounds<2>({p0, p1}, tl, br, r, w, h);
        if (p0.x < 0) p0 = p1;
        else if (p1.x < 0) p1 = p0;

        int r2 = r * r;
        for (int y = tl.y; y <= br.y; y++) {
            for (int x = tl.x; x <= br.x; x++) {
                double d2 = boundedLineDist2(p0, p1, {x, y});
                if (d2 < r2) {
                    dst[y * w + x] = color;
                }
            }
        }
    }

    void blit_canvas(uint32_t* pixels, int pitch, SDL_PixelFormat* format) {
        // Copy canvas to surface
        uint8_t* src_pixels = reinterpret_cast<uint8_t*>(canvas.data());
        uint8_t* dst_pixels = reinterpret_cast<uint8_t*>(pixels);
        for (int y = 0; y < h; y++) {
            SDL_memcpy(dst_pixels + y * pitch, 
                       src_pixels + y * w * sizeof(uint32_t), 
                       w * sizeof(uint32_t));
        }
    }

    void blit_cursor(uint32_t* pixels, int pitch, SDL_PixelFormat* format) {
        draw_circle(
            pixels,
            w, 
            h, 
            mouse_pos,
            radius,
            SDL_MapRGBA(format, color.r, color.g, color.b, 255),
            borderRadius,
            pitch // pitch is number of bytes
        );
    }
};

#endif