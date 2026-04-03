#ifndef PARTICLEBOX_PARTICLE
#define PARTICLEBOX_PARTICLE

#include <SDL.h>

#include <array>
#include <iostream>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "utils.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

struct Particle {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    u_int8_t r = 255, g = 255, b = 255;

    bool is_overlap(const Particle& other, float rad) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float d = rad + rad;
        return dx * dx + dy * dy < d * d;
    }

    void step() {
        x += vx;
        y += vy;
    }

    bool resolve_wall_collision(float w, float h, float rad) {
        bool resolved = false;
        if ((x + rad > w && vx > 0) || (x - rad < 0 && vx < 0)) {
            vx = -vx;
            resolved = true;
        }
        if ((y + rad > h && vy > 0) || (y - rad < 0 && vy < 0)) {
            vy = -vy;
            resolved = true;
        }
        return resolved;
    }

    friend bool resolve_collision(Particle& p1, Particle& p2) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float dvx = p2.vx - p1.vx;
        float dvy = p2.vy - p1.vy;
        float dot = dvx * dx + dvy * dy;
        if (dot >= 0) {
            return false;
        }

        float distance_sq = dx * dx + dy * dy;
        if (distance_sq <= 1e-6f) {
            return false;
        }

        float scale = dot / distance_sq;
        p1.vx += scale * dx;
        p1.vy += scale * dy;
        p2.vx -= scale * dx;
        p2.vy -= scale * dy;
        return true;
    }

    friend std::ostream& operator<<(std::ostream& o, const Particle& p) {
        o << "[p: (" << p.x << "," << p.y << "), v: (" << p.vx << "," << p.vy << "), r: "
          << static_cast<int>(p.r) << "]";
        return o;
    }
};

struct QuadRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    bool intersects(const QuadRect& other) const {
        return !(other.x > x + w || other.x + other.w < x || other.y > y + h || other.y + other.h < y);
    }
};

struct QuadTree {
    static constexpr int kDefaultCapacity = 8;
    static constexpr int kDefaultMaxDepth = 8;

    const std::vector<Particle>& particles;
    QuadRect boundary;
    int depth = 0;
    int capacity = kDefaultCapacity;
    int max_depth = kDefaultMaxDepth;
    std::vector<int> indices;
    std::array<std::unique_ptr<QuadTree>, 4> children;

    QuadTree(const std::vector<Particle>& particles, QuadRect boundary, int depth = 0,
             int capacity = kDefaultCapacity, int max_depth = kDefaultMaxDepth)
        : particles(particles), boundary(boundary), depth(depth), capacity(capacity), max_depth(max_depth) {}

    bool is_leaf() const {
        return children[0] == nullptr;
    }

    bool insert(int particle_idx) {
        const auto& p = (*particles)[particle_idx];
        if (!boundary.contains(p.x, p.y)) {
            return false;
        }

        if (static_cast<int>(indices.size()) < capacity || depth >= max_depth) {
            indices.push_back(particle_idx);
            return true;
        }

        if (is_leaf()) {
            subdivide();
        }

        for (auto& child : children) {
            if (child->insert(particle_idx)) {
                return true;
            }
        }

        // Fallback for precision edge cases near child boundaries.
        indices.push_back(particle_idx);
        return true;
    }

    void query(const QuadRect& range, std::vector<int>& out) const {
        if (!boundary.intersects(range)) {
            return;
        }

        for (int idx : indices) {
            const auto& p = (*particles)[idx];
            if (range.contains(p.x, p.y)) {
                out.push_back(idx);
            }
        }

        if (is_leaf()) {
            return;
        }

        for (const auto& child : children) {
            child->query(range, out);
        }
    }

    void subdivide() {
        float half_w = boundary.w * 0.5f;
        float half_h = boundary.h * 0.5f;
        float x = boundary.x;
        float y = boundary.y;

        children[0] = std::make_unique<QuadTree>(particles, QuadRect{x, y, half_w, half_h}, depth + 1, capacity, max_depth);
        children[1] = std::make_unique<QuadTree>(particles, QuadRect{x + half_w, y, half_w, half_h}, depth + 1, capacity, max_depth);
        children[2] = std::make_unique<QuadTree>(particles, QuadRect{x, y + half_h, half_w, half_h}, depth + 1, capacity, max_depth);
        children[3] = std::make_unique<QuadTree>(particles, QuadRect{x + half_w, y + half_h, half_w, half_h}, depth + 1, capacity, max_depth);

        auto moved_indices = std::move(indices);
        indices.clear();
        for (int idx : moved_indices) {
            insert(idx);
        }
    }
};

struct ParticleBox {
    SDL_Texture* tex = nullptr;
    SDL_Texture* particle_tex = nullptr;
    int w = 1000;
    int h = 1000;
    float rad = 1.0f;
    std::vector<Particle> particles;

    ParticleBox(SDL_Renderer* renderer, SDL_Texture* particle_tex, int w, int h)
        : particle_tex(particle_tex), w(w), h(h) {
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    }

    void meet_target(int target_num) {
        int req = target_num - static_cast<int>(particles.size());
        if (req > 0) {
            for (int i = 0; i < req; i++) {
                float spawn_rad = 10.0f;
                float x = get_rand_float(spawn_rad, w - spawn_rad);
                float y = get_rand_float(spawn_rad, h - spawn_rad);
                float vx = get_rand_float(-2.0f, 2.0f);
                float vy = get_rand_float(-2.0f, 2.0f);
                u_int8_t r = get_rand_int(20, 255);
                u_int8_t g = get_rand_int(20, 255);
                u_int8_t b = get_rand_int(20, 255);
                particles.emplace_back(Particle{x: x, y: y, vx: vx, vy: vy, r: r, g: g, b: b});
            }
        } else if (req < 0) {
            for (int i = 0; i < -req; i++) {
                particles.pop_back();
            }
        }
    }

    void step() {
        int n = static_cast<int>(particles.size());
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int i = 0; i < n; i++) {
            particles[i].step();
        }

        // Build potential collision pairs once from positions, then iterate for stable velocity resolution.
        QuadTree quadtree(&particles, QuadRect{0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)});
        for (int i = 0; i < n; i++) {
            quadtree.insert(i);
        }

        std::vector<std::pair<int, int>> candidate_pairs;
        candidate_pairs.reserve(static_cast<size_t>(n) * 8);
        float overlap_dist = rad * 2.0f;

#ifdef _OPENMP
        std::vector<std::vector<std::pair<int, int>>> thread_pairs(static_cast<size_t>(omp_get_max_threads()));
#pragma omp parallel
        {
            int tid = omp_get_thread_num();
            auto& local_pairs = thread_pairs[static_cast<size_t>(tid)];
            std::vector<int> nearby_indices;
            nearby_indices.reserve(32);

#pragma omp for schedule(static)
            for (int i = 0; i < n; i++) {
                const auto& p = particles[i];
                nearby_indices.clear();
                quadtree.query(QuadRect{p.x - overlap_dist, p.y - overlap_dist, overlap_dist * 2.0f, overlap_dist * 2.0f}, nearby_indices);
                for (int j : nearby_indices) {
                    if (j > i) {
                        local_pairs.emplace_back(i, j);
                    }
                }
            }
        }

        for (auto& local_pairs : thread_pairs) {
            candidate_pairs.insert(candidate_pairs.end(), local_pairs.begin(), local_pairs.end());
        }
#else
        std::vector<int> nearby_indices;
        nearby_indices.reserve(32);
        for (int i = 0; i < n; i++) {
            const auto& p = particles[i];
            nearby_indices.clear();
            quadtree.query(QuadRect{p.x - overlap_dist, p.y - overlap_dist, overlap_dist * 2.0f, overlap_dist * 2.0f}, nearby_indices);
            for (int j : nearby_indices) {
                if (j > i) {
                    candidate_pairs.emplace_back(i, j);
                }
            }
        }
#endif

        bool no_collision = false;
        int max_iterations = 100;
        while (!no_collision && max_iterations-- > 0) {
            no_collision = true;

            int wall_collisions = 0;
#ifdef _OPENMP
#pragma omp parallel for schedule(static) reduction(+ : wall_collisions)
#endif
            for (int i = 0; i < n; i++) {
                if (particles[i].resolve_wall_collision(w, h, rad)) {
                    wall_collisions += 1;
                }
            }
            if (wall_collisions > 0) {
                no_collision = false;
            }

            for (const auto& pair : candidate_pairs) {
                auto& p1 = particles[pair.first];
                auto& p2 = particles[pair.second];
                if (p1.is_overlap(p2, rad) && resolve_collision(p1, p2)) {
                    no_collision = false;
                }
            }
        }
    }

    SDL_Texture* render(SDL_Renderer* renderer) {
        SDL_SetRenderTarget(renderer, tex);
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        for (auto& p : particles) {
            SDL_SetTextureColorMod(particle_tex, p.r, p.g, p.b);
            SDL_FRect rect{p.x - rad, p.y - rad, rad * 2.0f, rad * 2.0f};
            SDL_RenderCopyF(renderer, particle_tex, nullptr, &rect);
        }
        SDL_SetRenderTarget(renderer, nullptr);
        return tex;
    }
};

#endif
