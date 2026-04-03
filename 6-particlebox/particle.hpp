#ifndef PARTICLEBOX_PARTICLE
#define PARTICLEBOX_PARTICLE

#include <SDL.h>

#include <array>
#include <iostream>
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

struct QuadTreeArena {
    struct Node {
        QuadRect boundary;
        int depth = 0;
        int children[4] = {-1, -1, -1, -1};
        int chunk = -1;       // first chunk index
        int chunk_tail = -1;  // last chunk index (for O(1) append)
        int total_count = 0;
    };

    // Each chunk: [capacity] index slots + 1 next-chunk slot.
    // chunk_pool layout: [idx0, idx1, ..., idx(cap-1), next_chunk] repeated.
    static constexpr int kDefaultCapacity = 8;
    static constexpr int kDefaultMaxDepth = 8;
    static constexpr int kInitialNodes = 8192;
    static constexpr int kInitialChunks = 16384;

    int chunk_stride = kDefaultCapacity + 1;

    std::vector<Node> nodes;
    std::vector<int> chunk_pool;  // flat array of chunks, each chunk_stride ints
    int node_count = 0;
    int chunk_count = 0;
    const std::vector<Particle>* particles = nullptr;
    int node_capacity = kDefaultCapacity;
    int max_depth = kDefaultMaxDepth;

    QuadTreeArena() {
        nodes.resize(kInitialNodes);
        chunk_pool.resize(kInitialChunks * chunk_stride);
    }

    void reset(const std::vector<Particle>& p, QuadRect root_boundary,
               int capacity = kDefaultCapacity, int max_depth_val = kDefaultMaxDepth) {
        particles = &p;
        node_capacity = capacity;
        chunk_stride = capacity + 1;
        max_depth = max_depth_val;
        node_count = 0;
        chunk_count = 0;
        alloc_node(root_boundary, 0);
    }

    int alloc_chunk() {
        int idx = chunk_count++;
        if (idx * chunk_stride + chunk_stride > static_cast<int>(chunk_pool.size())) {
            chunk_pool.resize(chunk_pool.size() * 2);
        }
        chunk_pool[idx * chunk_stride + node_capacity] = -1; // next = none
        return idx;
    }

    int alloc_node(QuadRect boundary, int depth) {
        int idx = node_count++;
        if (idx >= static_cast<int>(nodes.size())) {
            nodes.resize(nodes.size() * 2);
        }
        auto& node = nodes[idx];
        node.boundary = boundary;
        node.depth = depth;
        node.children[0] = node.children[1] = node.children[2] = node.children[3] = -1;
        node.chunk = -1;
        node.chunk_tail = -1;
        node.total_count = 0;
        return idx;
    }

    void push_index(int node_idx, int particle_idx) {
        auto& node = nodes[node_idx];
        int pos_in_chunk = node.total_count % node_capacity;
        if (pos_in_chunk == 0) {
            // Need a new chunk.
            int new_chunk = alloc_chunk();
            if (node.chunk_tail >= 0) {
                chunk_pool[node.chunk_tail * chunk_stride + node_capacity] = new_chunk;
            } else {
                node.chunk = new_chunk;
            }
            node.chunk_tail = new_chunk;
        }
        chunk_pool[node.chunk_tail * chunk_stride + pos_in_chunk] = particle_idx;
        node.total_count++;
    }

    bool is_leaf(int node_idx) const {
        return nodes[node_idx].children[0] == -1;
    }

    bool insert(int node_idx, int particle_idx) {
        const auto& p = (*particles)[particle_idx];
        if (!nodes[node_idx].boundary.contains(p.x, p.y)) {
            return false;
        }

        if (nodes[node_idx].total_count < node_capacity || nodes[node_idx].depth >= max_depth) {
            push_index(node_idx, particle_idx);
            return true;
        }

        if (is_leaf(node_idx)) {
            subdivide(node_idx);
        }

        for (int ci = 0; ci < 4; ci++) {
            if (insert(nodes[node_idx].children[ci], particle_idx)) {
                return true;
            }
        }

        // Fallback for precision edge cases near child boundaries.
        push_index(node_idx, particle_idx);
        return true;
    }

    void query(int node_idx, const QuadRect& range, std::vector<int>& out) const {
        const auto& node = nodes[node_idx];
        if (!node.boundary.intersects(range)) {
            return;
        }

        int remaining = node.total_count;
        int c = node.chunk;
        while (c >= 0 && remaining > 0) {
            int n = remaining < node_capacity ? remaining : node_capacity;
            const int* base = &chunk_pool[c * chunk_stride];
            for (int i = 0; i < n; i++) {
                const auto& p = (*particles)[base[i]];
                if (range.contains(p.x, p.y)) {
                    out.push_back(base[i]);
                }
            }
            remaining -= n;
            c = chunk_pool[c * chunk_stride + node_capacity];
        }

        if (is_leaf(node_idx)) {
            return;
        }

        for (int ci = 0; ci < 4; ci++) {
            query(node.children[ci], range, out);
        }
    }

    void subdivide(int node_idx) {
        float half_w = nodes[node_idx].boundary.w * 0.5f;
        float half_h = nodes[node_idx].boundary.h * 0.5f;
        float bx = nodes[node_idx].boundary.x;
        float by = nodes[node_idx].boundary.y;
        int next_depth = nodes[node_idx].depth + 1;

        // Collect old indices before re-inserting.
        int old_count = nodes[node_idx].total_count;
        int old_chunk = nodes[node_idx].chunk;

        int c0 = alloc_node(QuadRect{bx, by, half_w, half_h}, next_depth);
        int c1 = alloc_node(QuadRect{bx + half_w, by, half_w, half_h}, next_depth);
        int c2 = alloc_node(QuadRect{bx, by + half_h, half_w, half_h}, next_depth);
        int c3 = alloc_node(QuadRect{bx + half_w, by + half_h, half_w, half_h}, next_depth);

        nodes[node_idx].children[0] = c0;
        nodes[node_idx].children[1] = c1;
        nodes[node_idx].children[2] = c2;
        nodes[node_idx].children[3] = c3;
        nodes[node_idx].chunk = -1;
        nodes[node_idx].chunk_tail = -1;
        nodes[node_idx].total_count = 0;

        int remaining = old_count;
        int c = old_chunk;
        while (c >= 0 && remaining > 0) {
            int n = remaining < node_capacity ? remaining : node_capacity;
            for (int i = 0; i < n; i++) {
                insert(node_idx, chunk_pool[c * chunk_stride + i]);
            }
            remaining -= n;
            c = chunk_pool[c * chunk_stride + node_capacity];
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
    QuadTreeArena quadtree;
    std::vector<std::pair<int, int>> candidate_pairs;

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
        quadtree.reset(particles, QuadRect{0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)});
        for (int i = 0; i < n; i++) {
            quadtree.insert(0, i);
        }

        candidate_pairs.clear();
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
                quadtree.query(0, QuadRect{p.x - overlap_dist, p.y - overlap_dist, overlap_dist * 2.0f, overlap_dist * 2.0f}, nearby_indices);
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
            quadtree.query(0, QuadRect{p.x - overlap_dist, p.y - overlap_dist, overlap_dist * 2.0f, overlap_dist * 2.0f}, nearby_indices);
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
