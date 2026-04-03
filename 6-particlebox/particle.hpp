#ifndef PARTICLEBOX_PARTICLE
#define PARTICLEBOX_PARTICLE

#include <iostream>
#include <vector>
#include <random>
#include <SDL.h>

inline int get_rand_int(int l, int r) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> uniform_dist(l, r);
    return uniform_dist(gen);
}

inline float get_rand_float(float l, float r) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> uniform_dist(l, r);
    return uniform_dist(gen);
}

namespace Entity {
template <typename T, int W, int H>
struct StaticRenderable {
    inline static SDL_Texture* tex = NULL;

    void init_tex(SDL_Renderer* renderer) {
        if (tex != NULL) return;
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
        static_cast<T*>(this)->draw_static(tex, W, H);
    }

    void destroy_tex() {
        if (tex != NULL) {
            SDL_DestroyTexture(tex);
            tex = NULL;
        }
    }

    void draw_static(SDL_Texture* tex, int w, int h) {}

    void render(SDL_Renderer* renderer, SDL_FRect* dest_rect) {
        SDL_RenderCopyF(renderer, tex, NULL, dest_rect);
    }
};

template <typename T, int W, int H>
struct TargetRenderable {
    SDL_Texture* tex = NULL;

    void init_tex(SDL_Renderer* renderer) {
        if (tex != NULL) return;
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, W, H);
    }

    void destroy_tex() {
        if (tex != NULL) {
            SDL_DestroyTexture(tex);
            tex = NULL;
        }
    }

    void render(SDL_Renderer* renderer, SDL_FRect* dest_rect) {
        SDL_SetRenderTarget(renderer, tex);
        static_cast<T*>(this)->draw(renderer, W, H);
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopyF(renderer, tex, NULL, dest_rect);
    }
    
    void draw(SDL_Renderer* renderer, int w, int h) {}
};


struct Particle: StaticRenderable<Particle, 100, 100> {
    float x;
    float y;
    float vx;
    float vy;
    float r = 1.0;

    void draw_static(SDL_Texture* tex, int w, int h) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        std::vector<Uint32> pixels(w * h);
        int pitch = w * sizeof(Uint32);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float dx = (static_cast<float>(x) - w / 2) / (static_cast<float>(w) / 2.0f);
                float dy = (static_cast<float>(y) - h / 2) / (static_cast<float>(h) / 2.0f);
                if (dx * dx + dy * dy <= 1.0f) {
                    pixels[y * w + x] = 0xFFFFFFFF;
                } else {
                    pixels[y * w + x] = 0x00000000;
                }
            }
        }
        SDL_UpdateTexture(tex, NULL, pixels.data(), pitch);
    }
    
    Particle(float x, float y, float vx, float vy, float r = 1.0): x(x), y(y), vx(vx), vy(vy), r(r) {}

    bool is_overlap(const Particle& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float d = r + other.r;
        return dx * dx + dy * dy < d * d;
    }

    bool is_approaching(const Particle& other) const {
        float dpx = other.x - x;
        float dpy = other.y - y;
        float dvx = other.vx - vx;
        float dvy = other.vy - vy;
        return dpx * dvx + dpy * dvy < -0.0001f;
    }

    bool is_colliding(const Particle& other) const {
        return is_overlap(other) && is_approaching(other);
    }

    void step() {
        x += vx;
        y += vy;
    }

    bool resolve_wall_collision(float w, float h) {
        bool resolved = false;
        if ((x > w && vx > 0) || (x < 0 && vx < 0)) {
            vx = -vx;
            resolved |= true;
        }
        if ((y > h && vy > 0) || (y < 0 && vy < 0)) {
            vy = -vy;
            resolved |= true;
        }
        return resolved;
    }

    friend std::ostream& operator<<(std::ostream& o, const Particle& p) {
        o << "[p: (" << p.x << "," << p.y << "), v: (" << p.vx << "," << p.vy << "), r: " << p.r << "]";
        return o;
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
};

struct ParticleBox: TargetRenderable<ParticleBox, 1280, 800> {
    float w;
    float h;
    std::vector<Particle> particles;

    ParticleBox(): w(10000), h(10000) {}
    ParticleBox(int w, int h): w(w), h(h) {}

    void add_particle(const Particle& p) {
        particles.push_back(p);
    }

    void random_init(int num_particles) {
        for (int i = 0; i < num_particles; i++) {
            float x = get_rand_float(0.0, w);
            float y = get_rand_float(0.0, h);
            float vx = get_rand_float(0, 2.0);
            float vy = get_rand_float(0, 2.0);
            // float vx = 0;
            // float vy = 0;
            particles.push_back(Particle(x, y, vx, vy, 10.0));
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

    void draw(SDL_Renderer* renderer, int w, int h) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        for (auto& p: particles) {
            float size = p.r * 2;
            SDL_FRect rect{p.x - p.r, p.y - p.r, size, size};
            p.render(renderer, &rect);
        }
    }
};


}

#endif