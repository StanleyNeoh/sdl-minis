#ifndef RAYTRACER_CAMERA
#define RAYTRACER_CAMERA

#include <iostream>
#include "vec3.hpp"
#include "ray.hpp"
#include "utils.hpp"
#include "hittable.hpp"
#include <SDL.h>

// Take z as forward
// Take y as down
// Take x as right

template <int w, int h>
struct Camera {
    static inline constexpr int mind = std::min(w, h);
    const Hittables& hittables;

    // State
    float mouse_sens = 0.005f;
    Interval search_range = {0.001, 20};
    float vp_focal_length = 1.0;
    float fov = 90;
    float aperture_rad = 0.0f;
    float vp_h;
    float vp_w;
    Vec3 center = {0, 0, 0};
    Vec3 vp_u = {1, 0, 0};    // points right on viewport
    Vec3 vp_v = {0, 1, 0};    // points down on viewport
    float yaw = 0.0f;
    float pitch = 0.0f;
    int stride = 1;
    
    // Cached
    Vec3 pixel00_loc;
    std::vector<Vec3> colors = std::vector<Vec3>(w * h, Vec3{0, 0, 0});
    int n_since_move = 0;

    Camera(const Hittables& hittables): hittables(hittables) {
        update_vp();
    }

    Vec3 forward_dir() {
        return vp_u.cross(vp_v);
    }

    void update_vp() {
        vp_h = 2 * std::tan(fov / 360 * M_PI) * vp_focal_length;
        vp_w = (static_cast<float>(w) / h) * vp_h;
    }

    void auto_focus() {
        Ray ray(center, forward_dir());
        HitRecord record;
        if (hittables.hit(ray, search_range, record)) {
            vp_focal_length = record.t;
            update_vp();
        };
    }

    void handle_keycode(SDL_Keycode keycode) {
        switch (keycode) {
        case SDLK_EQUALS:
            stride--;
            if (stride < 1) stride = 1;
            break;
        case SDLK_MINUS:
            stride++;
            if (stride > mind) stride = mind;
            break;
        case SDLK_RIGHTBRACKET:
            fov--;
            if (fov < 5) fov = 5;  
            update_vp();
            break;
        case SDLK_LEFTBRACKET:
            fov++;
            if (fov > 150) fov = 150;  
            update_vp();
            break;
        case SDLK_QUOTE:
            aperture_rad += 0.001;
            break;
        case SDLK_SEMICOLON:
            aperture_rad -= 0.001;
            if (aperture_rad < 0) aperture_rad = 0.0f;
            break;
        case SDLK_f:
            auto_focus();
            break;
        default:
            return;
        }
        n_since_move = 0;
    }

    void handle_event(const SDL_Event& event) {
        if (event.type == SDL_MOUSEMOTION) {
            if (event.motion.state & SDL_BUTTON_LMASK) {
                yaw += event.motion.xrel * mouse_sens;
                pitch -= event.motion.yrel * mouse_sens;
                n_since_move = 0;

                // clamp pitch
                if (pitch > 1.5f) pitch = 1.5f;
                if (pitch < -1.5f) pitch = -1.5f;
            }
        } else if (event.type == SDL_KEYDOWN) {
            handle_keycode(event.key.keysym.sym);
        }
    }

    void update(float dt) {
        vp_u = {
            std::cos(yaw),
            0,
            -std::sin(yaw),
        };
        vp_v = {
            std::sin(yaw) * std::sin(pitch),
            std::cos(pitch),
            std::cos(yaw) * std::sin(pitch)
        };
        Vec3 forward = forward_dir();
        Vec3 scaled_u = vp_w * vp_u;
        Vec3 scaled_v = vp_h * vp_v;
        pixel00_loc = forward * vp_focal_length - (scaled_u / 2) - (scaled_v / 2) + 0.5 * (scaled_u / w + scaled_v / h);
        forward.y = 0;
        forward.normalize();

        const Uint8* state = SDL_GetKeyboardState(NULL);
        float speed = 10.0f * dt;
        if (state[SDL_SCANCODE_W]) {
            center += forward * speed;
            n_since_move = 0;
        }
        if (state[SDL_SCANCODE_S]) {
            center -= forward * speed;
            n_since_move = 0;
        }
        if (state[SDL_SCANCODE_D]) {
            center += vp_u * speed;
            n_since_move = 0;
        }
        if (state[SDL_SCANCODE_A]) {
            center -= vp_u * speed;
            n_since_move = 0;
        }
        if (state[SDL_SCANCODE_SPACE]) {
            center.y -= speed;
            n_since_move = 0;
        }
        if (state[SDL_SCANCODE_LCTRL]) {
            center.y += speed;
            n_since_move = 0;
        }
        if (stride < 1) stride = 1;
        if (stride > mind) stride = mind;
    }

    void scan(Uint32* pixels, int pitch) {
        Vec3 pix_u = (vp_w / w) * vp_u;
        Vec3 pix_v = (vp_h / h) * vp_v;
        n_since_move++;

        int n_hchunk = (h + stride - 1) / stride;
        int n_wchunk = (w + stride - 1) / stride;
        float stride_adjust_r = (vp_h / h) * (stride - 1) / 2.0f;
        float stride_adjust_c = (vp_w / w) * (stride - 1) / 2.0f;
        int nbounce = n_since_move == 1 ? 10 : 100;

        #pragma omp parallel
        {
            #pragma omp for schedule(static)
            for (int i = 0; i < n_hchunk * n_wchunk; i++) {
                int r = i / n_wchunk;
                int c = i % n_wchunk;

                int ri = stride * r;
                int ci = stride * c;
                int rl = std::min(stride, h - ri);
                int cl = std::min(stride, w - ci);
                int ind = ri * w + ci;
                float rf = ri + stride_adjust_r;
                float cf = ci + stride_adjust_c;
                if (n_since_move == 1) {
                    colors[ind] = {0, 0, 0};
                } else {
                    rf += (random_float() - 0.5) * stride;
                    cf += (random_float() - 0.5) * stride;
                }

                float du, dv;
                vec2_random(du, dv, aperture_rad);
                Vec3 defocus_offset = du * vp_u + dv * vp_v;
                Vec3 dir = (pixel00_loc + rf * pix_v + cf * pix_u - defocus_offset).unit();
                Ray ray{center + defocus_offset, dir};
                colors[ind] += hittables.get_color(ray, search_range, nbounce);
            }

            if (stride > 1) {
                #pragma omp barrier
                #pragma omp for schedule(static)
                for (int i = 0; i < h * w; i++) {
                    int r = i / w;
                    int c = i % w;
                    int ind = r * w + c;
                    int rbase = r - (r % stride);
                    int cbase = c - (c % stride);
                    int indbase = rbase * w + cbase;
                    int rbase_nxt = rbase + stride;
                    int cbase_nxt = cbase + stride;
                    if (rbase_nxt < h && cbase_nxt < w) {
                        colors[ind] = (
                            (rbase_nxt - r) * (cbase_nxt - c) * colors[indbase] 
                            + (r - rbase) * (cbase_nxt - c) * colors[rbase_nxt * w + c]
                            + (rbase_nxt - r) * (c - cbase) * colors[rbase * w + cbase_nxt]
                            + (r - rbase) * (c - cbase) * colors[rbase_nxt * w + cbase_nxt]
                        ) / (stride * stride);
                    } else {
                        colors[ind] = colors[indbase];
                    }
                }
            }

            #pragma omp for schedule(static)
            for (int i = 0; i < h * w; i++) {
                pixels[i] = (colors[i] / n_since_move).as_argb();
            }
        }
    }

    void render(SDL_Texture* screen_tex) {
        Uint32* pixels;
        int pitch;
        SDL_LockTexture(screen_tex, NULL, (void**)&pixels, &pitch);
        scan(pixels, pitch);
        SDL_UnlockTexture(screen_tex);
    }
};

#endif
