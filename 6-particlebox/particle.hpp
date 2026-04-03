#ifndef PARTICLEBOX_PARTICLE
#define PARTICLEBOX_PARTICLE

#include <iostream>
#include <vector>
#include <random>
#include <SDL.h>
#include "utils.hpp"

struct Particle {
    float x = 0.0;
    float y = 0.0;
    float vx = 0.0;
    float vy = 0.0;
    float rad = 1.0;
    u_int8_t r = 255, g = 255, b = 255;

    bool is_overlap(const Particle& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float d = rad + other.rad;
        return dx * dx + dy * dy < d * d;
    }

    void step() {
        x += vx;
        y += vy;
    }

    bool resolve_wall_collision(float w, float h) {
        bool resolved = false;
        if ((x + rad > w && vx > 0) || (x - rad < 0 && vx < 0)) {
            vx = -vx;
            resolved |= true;
        }
        if ((y + rad > h && vy > 0) || (y - rad < 0 && vy < 0)) {
            vy = -vy;
            resolved |= true;
        }
        return resolved;
    }

    friend bool resolve_collision(Particle& p1, Particle& p2) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float dvx = p2.vx - p1.vx;
        float dvy = p2.vy - p1.vy;
        float dot = dvx * dx + dvy * dy;
        if (dot >= 0) return false;
        float scale = dot / (dx * dx + dy * dy);
        p1.vx += scale * dx;
        p1.vy += scale * dy;
        p2.vx -= scale * dx;
        p2.vy -= scale * dy;
        return true;
    }

    friend std::ostream& operator<<(std::ostream& o, const Particle& p) {
        o << "[p: (" << p.x << "," << p.y << "), v: (" << p.vx << "," << p.vy << "), r: " << p.r << "]";
        return o;
    }
};

struct ParticleBox {
    SDL_Texture* tex = NULL;
    SDL_Texture* particle_tex = NULL;
    int w = 1000;
    int h = 1000;
    std::vector<Particle> particles;

    ParticleBox(SDL_Renderer* renderer, SDL_Texture* particle_tex, int w, int h): particle_tex(particle_tex), w(w), h(h) {
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    }

    void random_init(int num_particles) {
        for (int i = 0; i < num_particles; i++) {
            float rad = 10.0;
            float x = get_rand_float(rad, w - rad);
            float y = get_rand_float(rad, h - rad);
            float vx = get_rand_float(-2.0, 2.0);
            float vy = get_rand_float(-2.0, 2.0);
            u_int8_t r = get_rand_int(20, 255);
            u_int8_t g = get_rand_int(20, 255);
            u_int8_t b = get_rand_int(20, 255);
            particles.emplace_back(Particle{x: x, y: y, vx: vx, vy: vy, rad: 10.0, r: r, g: g, b: b});
        }
    }

    void step() {
        int n = particles.size();
        int no_collision = false;
        for (int i = 0; i < n; i++) {
            particles[i].step();
        }

        while (!no_collision) {
            no_collision = true;
            for (int i = 0; i < n; i++) {
                if (particles[i].resolve_wall_collision(w, h)) {
                    no_collision = false;
                }
            }

            for (int i = 0; i < n; i++) {
                for (int j = i+1; j < n; j++) {
                    auto& p1 = particles[i];
                    auto& p2 = particles[j];
                    if (p1.is_overlap(p2) && resolve_collision(p1, p2)) {
                        no_collision = false;
                    }
                }
            }
        }
    }

    SDL_Texture* render(SDL_Renderer* renderer) {
        SDL_SetRenderTarget(renderer, tex);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        for (auto& p: particles) {
            SDL_SetTextureColorMod(particle_tex, p.r, p.g, p.b);
            SDL_FRect rect{p.x - p.rad, p.y - p.rad, p.rad * 2, p.rad * 2};
            SDL_RenderCopyF(renderer, particle_tex, NULL, &rect);
        }
        SDL_SetRenderTarget(renderer, NULL);
        return tex;
    }
};

#endif