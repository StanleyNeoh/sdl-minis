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
    
    void reset(const Particles& _particles) {
        particles = &_particles;
        int n = particles->size();
        if (!is_leaf()) {
            for (int i = 0; i < 4; i++) children[i].reset();
        }
        indices.clear();
        for (int pi = 0; pi < n; pi++) insert(pi);
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

        for (int pj: indices) {
            const auto& p = (*particles)[pj];
            if (pj > pi && range.contains(p.x, p.y)) {
                out.emplace_back(pi, pj);
            }
        }

        if (is_leaf()) return;

        for (int ci = 0; ci < 4; ci++) {
            children[ci]->query_nearby_pi(pi, range, out);
        }
    }

    void query_nearby_pairs(float check_rad, vpii& out) const {
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
                    query_nearby_pi(pi, QuadBox{p.x - check_rad, p.y - check_rad, 2 * check_rad, 2 * check_rad}, thread_pairs);
                }
            }
            for (auto& thread_pairs: nearby_thread_pairs) {
                out.insert(out.end(), thread_pairs.begin(), thread_pairs.end());
            }
        #else
            for (int pi = 0; pi < n; pi++) {
                const auto& p = (*particles)[pi];
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
};

#endif