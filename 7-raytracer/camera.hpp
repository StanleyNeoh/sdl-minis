#ifndef RAYTRACER_CAMERA
#define RAYTRACER_CAMERA

#include <iostream>
#include "vec3.hpp"
#include "ray.hpp"
#include "utils.hpp"
#include <SDL.h>

// Take z as forward
// Take y as down
// Take x as right

struct Camera {
    int w;
    int h;
    Vec3 center = {0, 0, 0};

    float vp_focal_length = 1.0;
    float vp_h = 2.0;
    float vp_w;
    Vec3 vp_u = {1, 0, 0};    // points right on viewport
    Vec3 vp_v = {0, 1, 0};    // points down on viewport

    float yaw = 0.0f;
    float pitch = 0.0f;
    float mouse_sens = 0.005f;

    Vec3 forward_dir() {
        return vp_u.cross(vp_v);
    }

    Camera(int w, int h): w(w), h(h) {
        vp_w = static_cast<float>(w) / h * vp_h;
    }

    void handle_event(const SDL_Event& event) {
        if (event.type == SDL_MOUSEMOTION) {
            if (event.motion.state & SDL_BUTTON_LMASK) {
                yaw += event.motion.xrel * mouse_sens;
                pitch -= event.motion.yrel * mouse_sens;

                // clamp pitch
                if (pitch > 1.5f) pitch = 1.5f;
                if (pitch < -1.5f) pitch = -1.5f;
            }
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
        forward.y = 0;
        forward.normalize();

        const Uint8* state = SDL_GetKeyboardState(NULL);
        float speed = 10.0f * dt;
        if (state[SDL_SCANCODE_W]) center += forward * speed;
        if (state[SDL_SCANCODE_S]) center -= forward * speed;
        if (state[SDL_SCANCODE_D]) center += vp_u * speed;
        if (state[SDL_SCANCODE_A]) center -= vp_u * speed;
        if (state[SDL_SCANCODE_SPACE]) center.y -= speed;
        if (state[SDL_SCANCODE_LCTRL]) center.y += speed;
    }

    void scan(Uint32* pixels, int pitch, const std::vector<Hittable*>& hittables) {
        float halfh = h / 2.0;
        float halfw = w / 2.0;
        Vec3 scaled_u = (vp_w / w) * vp_u;
        Vec3 scaled_v = (vp_h / h) * vp_v;
        Vec3 dz = forward_dir() * vp_focal_length;
        #ifdef _OPENMP
            #pragma omp parallel for schedule(static)
            for (int i = 0; i < h * w; i++) {
                int r = i / w;
                int c = i % w;
                Uint32* row = offset(pixels, r * pitch);
                Vec3 dir = dz + (static_cast<float>(r) - halfh) * scaled_v + (static_cast<float>(c) - halfw) * scaled_u;
                Ray ray{center, dir.unit()};
                ray.cast(hittables);
                row[c] = ray.argb_color();
            }
        #else
            for (int i = 0; i < h; i++) {
                Uint32* row = offset(pixels, i * pitch);
                Vec3 dir = dz + (static_cast<float>(i) - halfh) * scaled_v;
                for (int j = 0; j < w; j++) {
                    Vec3 dy = (static_cast<float>(j) - halfw) * scaled_u;
                    Ray ray{center, (dir + dy).unit()};
                    ray.cast(hittables);
                    row[j] = ray.argb_color();
                }
            }
        #endif
    }

    void render(SDL_Texture* screen_tex, const std::vector<Hittable*>& hittables) {
        Uint32* pixels;
        int pitch;
        SDL_LockTexture(screen_tex, NULL, (void**)&pixels, &pitch);
        scan(pixels, pitch, hittables);
        SDL_UnlockTexture(screen_tex);
    }
};

#endif
