#ifndef PARTICLEBOX_QUADTREE
#define PARTICLEBOX_QUADTREE

#include <memory>
#include <vector>
#include <array>
#include "particle.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

struct QuadBox {
    float x;
    float y;
    float w;
    float h;

    bool contains(float _x, float _y) const {
        return (_x >= x && _x < x + w && _y >= y && _y < y + h);
    }

    bool overlap(const QuadBox& other) const {
        return !(
            x + w <= other.x || other.x + other.w <= x ||
            y + h <= other.y || other.y + other.h <= y
        );
    }

    bool inside(const QuadBox& other) const {
        return (
            x > other.x && x + w < other.x + other.w &&
            y > other.y && y + h < other.y + other.h
        );
    }
};

struct QuadTree {
    static constexpr int max_capacity = 8;
    static constexpr int max_depth = 8;
    using vpii = std::vector<std::pair<int, int>>;
    using vi = std::vector<int>;
    using Particles = std::vector<Particle>;

    QuadBox boundary;
    int depth = 0;

    vi indices;
    const Particles* particles = nullptr;
    std::array<std::unique_ptr<QuadTree>, 4> children;

    QuadTree(
        const QuadBox& boundary, 
        int depth = 0,
        const Particles* particles = nullptr
    ): boundary(boundary), depth(depth), particles(particles) {}
    
    void reset(Particles& _particles) {
        particles = &_particles;
        int n = particles->size();
        if (!is_leaf()) {
            for (int i = 0; i < 4; i++) children[i].reset();
        }
        indices.clear();
        for (int pi = 0; pi < n; pi++) {
            auto& p = _particles[pi];
            p.resolve_wall_collision(boundary.w, boundary.h);
            insert(pi);
        }
    }

    bool is_leaf() const {
        return children[0] == nullptr;
    }

    bool insert(int pi) {
        const auto& p = (*particles)[pi];
        if (!boundary.contains(p.x, p.y)) return false;
        if (indices.size() < max_capacity || depth >= max_depth) {
            indices.push_back(pi);
            return true;
        }

        if (is_leaf()) subdivide();

        for (int ci = 0; ci < 4; ci++) {
            if (children[ci]->boundary.contains(p.x, p.y)) {
                children[ci]->insert(pi);
                return true;
            }
        }
        indices.push_back(pi);
        return true;
    }

    void subdivide() {
        float x = boundary.x;
        float y = boundary.y;
        float halfw = boundary.w / 2;
        float halfh = boundary.h / 2;
        children[0] = std::make_unique<QuadTree>(QuadBox{x, y, halfw, halfh}, depth+1, particles);
        children[1] = std::make_unique<QuadTree>(QuadBox{x + halfw, y, halfw, halfh}, depth+1, particles);
        children[2] = std::make_unique<QuadTree>(QuadBox{x, y + halfh, halfw, halfh}, depth+1, particles);
        children[3] = std::make_unique<QuadTree>(QuadBox{x + halfw, y + halfh, halfw, halfh}, depth+1, particles);

        vi leftovers;
        for (int pi: indices) {
            const auto& p = (*particles)[pi];
            bool placed = false;
            for (int ci = 0; ci < 4; ci++) {
                if (children[ci]->boundary.contains(p.x, p.y)) {
                    children[ci]->insert(pi);
                    placed = true;
                    break;
                }
            }
            if (!placed) leftovers.push_back(pi);
        }
        indices = std::move(leftovers);
    }

    void query_nearby_pi(int pi, const QuadBox& range, vpii& out) const {
        if (!boundary.overlap(range)) return;

        const auto& pi_p = (*particles)[pi];
        for (int pj: indices) {
            if (pj == pi) continue;
            const auto& pj_p = (*particles)[pj];
            if (pi_p.rad < pj_p.rad || (pi_p.rad == pj_p.rad && pi > pj)) continue;
            if (range.contains(pj_p.x, pj_p.y)) {
                out.emplace_back(pi, pj);
            }
        }

        if (is_leaf()) return;

        for (int ci = 0; ci < 4; ci++) {
            children[ci]->query_nearby_pi(pi, range, out);
        }
    }

    void query_nearby_pairs(vpii& out) const {
        int n = particles->size();
        #ifdef _OPENMP
            std::vector<vpii> nearby_thread_pairs(omp_get_max_threads());
            #pragma omp parallel
            {
                int tid = omp_get_thread_num();
                auto& thread_pairs = nearby_thread_pairs[tid];
                #pragma omp for schedule(static)
                for (int pi = 0; pi < n; pi++) {
                    const auto& p = (*particles)[pi];
                    float check_rad = 2.0f * p.rad;
                    query_nearby_pi(pi, QuadBox{p.x - check_rad, p.y - check_rad, 2 * check_rad, 2 * check_rad}, thread_pairs);
                }
            }
            for (auto& thread_pairs: nearby_thread_pairs) {
                out.insert(out.end(), thread_pairs.begin(), thread_pairs.end());
            }
        #else
            for (int pi = 0; pi < n; pi++) {
                const auto& p = (*particles)[pi];
                float check_rad = 2.0f * p.rad;
                query_nearby_pi(pi, QuadBox{p.x - check_rad, p.y - check_rad, 2 * check_rad, 2 * check_rad}, out);
            }
        #endif
    }

    void _query_nearby_wall(const QuadBox& interior, vi& out) const {
        if (boundary.inside(interior)) return;
        for (int pi: indices) {
            const auto& p = (*particles)[pi];
            if (interior.contains(p.x, p.y)) continue;
            out.push_back(pi);
        }
        if (is_leaf()) return;
        for (int ci = 0; ci < 4; ci++) {
            children[ci]->_query_nearby_wall(interior, out);
        }
    }

    void query_nearby_wall(float check_pad, vi& out) const {
        _query_nearby_wall(QuadBox{boundary.x + check_pad, boundary.y + check_pad, boundary.w - 2 * check_pad, boundary.h - 2 * check_pad}, out);
    }

    int pool_node_used() const { return -1; }
    int pool_node_capacity() const { return -1; }
    int pool_indices_used() const { return -1; }
    int pool_indices_capacity() const { return -1; }
};

struct QuadTreeArena {
    static constexpr int max_capacity = 8;
    static constexpr int max_depth = 8;
    static constexpr int indices_chunk_size = max_capacity + 1;
    static constexpr int default_node_pool_size = 8000;
    static constexpr int default_indices_pool_size = default_node_pool_size * indices_chunk_size;

    struct Node {
        QuadBox boundary;
        int depth = 0;

        int indices_start;
        int indices_i;
        int indices_len;
        std::array<int, 4> children = {-1, -1, -1, -1};
    };

    using vpii = std::vector<std::pair<int, int>>;
    using vi = std::vector<int>;
    using Particles = std::vector<Particle>;

    const Particles* particles = nullptr;
    int root_node_i = -1;
    std::vector<Node> node_pool;
    std::vector<int> indices_pool;
    int indices_pool_i = 0;
    int node_pool_i = 0;

    int alloc_indices_chunk() {
        if (indices_pool_i + indices_chunk_size > static_cast<int>(indices_pool.size())) {
            indices_pool.resize(indices_pool.size() * 2);
        }
        int i = indices_pool_i;
        indices_pool_i += indices_chunk_size;
        for (int j = 0; j < indices_chunk_size; j++) indices_pool[i + j] = -1;
        return i;
    }

    int alloc_indices_chunk(int node_i) {
        auto& node = node_pool[node_i];
        if (node.indices_start < 0) {
            int i = alloc_indices_chunk();
            node.indices_start = i;
            node.indices_i = i;
            node.indices_len = 0;
            return i;
        }
        int chunk_start_i = node.indices_i - (node.indices_i % indices_chunk_size);
        int chunk_last_i = chunk_start_i + indices_chunk_size - 1;
        if (indices_pool[chunk_last_i] == -1) {
            int i = alloc_indices_chunk();
            indices_pool[chunk_last_i] = i;
            return i;
        }
        return -1;
    }

    int alloc_node(const QuadBox& boundary, int depth=0) {
        if (node_pool_i >= static_cast<int>(node_pool.size())) {
            node_pool.resize(node_pool.size() * 2);
        }
        int node_i = node_pool_i++;
        auto& node = node_pool[node_i];
        node.boundary = boundary;
        node.depth = depth;
        node.indices_start = -1;
        node.children = {-1, -1, -1, -1};
        alloc_indices_chunk(node_i);
        return node_i;
    }

    bool is_leaf(int node_i) const {
        return node_pool[node_i].children[0] == -1;
    }

    void indices_push_back(int node_i, int pi) {
        auto& node = node_pool[node_i];
        if (node.indices_i % indices_chunk_size == indices_chunk_size - 1) {
            alloc_indices_chunk(node_i);
            node.indices_i = indices_pool[node.indices_i];
        }
        indices_pool[node.indices_i] = pi;
        node.indices_i++;
        node.indices_len++;
    }

    void indices_clear(int node_i) {
        auto& node = node_pool[node_i];
        node.indices_i = node.indices_start;
        node.indices_len = 0;
    }

    bool indices_insert(int node_i, int pi) {
        const auto& p = (*particles)[pi];
        auto& node = node_pool[node_i];
        if (!node.boundary.contains(p.x, p.y)) return false;
        if (node.indices_len < max_capacity || node.depth >= max_depth) {
            indices_push_back(node_i, pi);
            return true;
        }

        if (is_leaf(node_i)) subdivide(node_i);

        for (int ci = 0; ci < 4; ci++) {
            int child_i = node.children[ci];
            if (node_pool[child_i].boundary.contains(p.x, p.y)) {
                indices_insert(child_i, pi);
                return true;
            }
        }
        indices_push_back(node_i, pi);
        return true;
    }

    void subdivide(int node_i) {
        auto& node = node_pool[node_i];
        float x = node.boundary.x;
        float y = node.boundary.y;
        float halfw = node.boundary.w / 2;
        float halfh = node.boundary.h / 2;
        node.children[0] = alloc_node(QuadBox{x, y, halfw, halfh}, node.depth+1);
        node.children[1] = alloc_node(QuadBox{x + halfw, y, halfw, halfh}, node.depth+1);
        node.children[2] = alloc_node(QuadBox{x, y + halfh, halfw, halfh}, node.depth+1);
        node.children[3] = alloc_node(QuadBox{x + halfw, y + halfh, halfw, halfh}, node.depth+1);

        int left = node.indices_len;
        int chunk_start = node.indices_start;
        indices_clear(node_i);
        while (left > 0) {
            int chunk_size = std::min(max_capacity, left);
            for (int i = 0; i < chunk_size; i++) {
                int pi = indices_pool[chunk_start + i];
                const auto& p = (*particles)[pi];
                bool placed = false;
                for (int ci = 0; ci < 4; ci++) {
                    int child_i = node.children[ci];
                    if (node_pool[child_i].boundary.contains(p.x, p.y)) {
                        indices_insert(child_i, pi);
                        placed = true;
                        break;
                    }
                }
                if (!placed) indices_push_back(node_i, pi);
            }
            left -= chunk_size;
            chunk_start = indices_pool[chunk_start + indices_chunk_size - 1];
        }
    }

    void query_nearby_pi(int node_i, int pi, const QuadBox& range, vpii& out) const {
        auto& node = node_pool[node_i];
        if (!node.boundary.overlap(range)) return;

        const auto& pi_p = (*particles)[pi];
        int left = node.indices_len;
        int chunk_start = node.indices_start;
        while (left > 0) {
            int chunk_size = std::min(max_capacity, left);
            for (int i = 0; i < chunk_size; i++) {
                int pj = indices_pool[chunk_start + i];
                if (pj == pi) continue;
                const auto& pj_p = (*particles)[pj];
                if (pi_p.rad < pj_p.rad || (pi_p.rad == pj_p.rad && pi > pj)) continue;
                if (range.contains(pj_p.x, pj_p.y)) {
                    out.emplace_back(pi, pj);
                }
            }
            left -= chunk_size;
            chunk_start = indices_pool[chunk_start + indices_chunk_size - 1];
        }

        if (is_leaf(node_i)) return;
        for (int ci = 0; ci < 4; ci++) {
            query_nearby_pi(node.children[ci], pi, range, out);
        }
    }

    void query_nearby_wall(int node_i, const QuadBox& interior, vi& out) const {
        auto& node = node_pool[node_i];
        if (node.boundary.inside(interior)) return;
        int left = node.indices_len;
        int chunk_start = node.indices_start;
        while (left > 0) {
            int chunk_size = std::min(max_capacity, left);
            for (int i = 0; i < chunk_size; i++) {
                int pi = indices_pool[chunk_start + i];
                const auto& p = (*particles)[pi];
                if (interior.contains(p.x, p.y)) continue;
                out.push_back(pi);
            }
            left -= chunk_size;
            chunk_start = indices_pool[chunk_start + indices_chunk_size - 1];
        }
        if (is_leaf(node_i)) return;
        for (int ci = 0; ci < 4; ci++) {
            query_nearby_wall(node.children[ci], interior, out);
        }
    }

    QuadTreeArena(const QuadBox& boundary, int depth = 0) {
        node_pool.resize(default_node_pool_size);
        indices_pool.resize(default_indices_pool_size);
        root_node_i = alloc_node(boundary, depth);
    }
    
    void reset(Particles& _particles) {
        particles = &_particles;
        int n = particles->size();
        // root_node always at index 0
        root_node_i = 0;
        node_pool_i = 1;
        indices_pool_i = indices_chunk_size;
        auto& node = node_pool[root_node_i];
        node.indices_start = 0;
        node.indices_i = 0;
        node.indices_len = 0;
        node.children = {-1, -1, -1, -1};
        for (int j = 0; j < indices_chunk_size; j++) indices_pool[j] = -1;
        for (int pi = 0; pi < n; pi++) {
            auto& p = _particles[pi];
            p.resolve_wall_collision(node.boundary.w, node.boundary.h);
            indices_insert(root_node_i, pi);
        }
    }

    void query_nearby_pairs(vpii& out) const {
        int n = particles->size();
        #ifdef _OPENMP
            std::vector<vpii> nearby_thread_pairs(omp_get_max_threads());
            #pragma omp parallel
            {
                int tid = omp_get_thread_num();
                auto& thread_pairs = nearby_thread_pairs[tid];
                #pragma omp for schedule(static)
                for (int pi = 0; pi < n; pi++) {
                    const auto& p = (*particles)[pi];
                    float check_rad = 2.0f * p.rad;
                    query_nearby_pi(root_node_i, pi, QuadBox{p.x - check_rad, p.y - check_rad, 2 * check_rad, 2 * check_rad}, thread_pairs);
                }
            }
            for (auto& thread_pairs: nearby_thread_pairs) {
                out.insert(out.end(), thread_pairs.begin(), thread_pairs.end());
            }
        #else
            for (int pi = 0; pi < n; pi++) {
                const auto& p = (*particles)[pi];
                float check_rad = 2.0f * p.rad;
                query_nearby_pi(root_node_i, pi, QuadBox{p.x - check_rad, p.y - check_rad, 2 * check_rad, 2 * check_rad}, out);
            }
        #endif
    }

    void query_nearby_wall(float check_pad, vi& out) const {
        auto& node = node_pool[root_node_i];
        query_nearby_wall(root_node_i, QuadBox{node.boundary.x + check_pad, node.boundary.y + check_pad, node.boundary.w - 2 * check_pad, node.boundary.h - 2 * check_pad}, out);
    }

    int pool_node_used() const { return node_pool_i; }
    int pool_node_capacity() const { return static_cast<int>(node_pool.size()); }
    int pool_indices_used() const { return indices_pool_i; }
    int pool_indices_capacity() const { return static_cast<int>(indices_pool.size()); }
};
#endif