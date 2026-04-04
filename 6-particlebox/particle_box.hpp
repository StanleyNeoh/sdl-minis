#ifndef PARTICLEBOX_PARTICLEBOX
#define PARTICLEBOX_PARTICLEBOX

#include <SDL.h>

#include <array>
#include <random>
#include <utility>
#include <vector>

#include "particle.hpp"
#include "utils.hpp"
#include "quadtree.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

struct ParticleBox {
    SDL_Texture* tex = nullptr;
    SDL_Texture* particle_tex = nullptr;
    int w = 1000;
    int h = 1000;
    float rad = 1.0f;
    QuadTree quadtree;
    std::vector<Particle> particles;

    ParticleBox(SDL_Renderer* renderer, SDL_Texture* particle_tex, int w, int h)
        : particle_tex(particle_tex), w(w), h(h), quadtree(QuadBox{0.0, 0.0, static_cast<float>(w), static_cast<float>(h)}) {
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
        int n = particles.size();
        for (int i = 0; i < n; i++) {
            particles[i].step();
        }

        std::vector<int> nearby_walls;
        std::vector<std::pair<int, int>> nearby_pairs;
        quadtree.reset(particles);
        quadtree.query_nearby_wall(rad, nearby_walls);
        quadtree.query_nearby_pairs(rad * 2.0f, nearby_pairs);

        #ifdef _OPENMP
            // Greedy graph coloring: partition pairs into conflict-free batches
            // Two pairs conflict if they share a particle index
            int n_pairs = nearby_pairs.size();
            std::vector<int> pair_color(n_pairs, -1);
            std::vector<int> particle_max_color(n, -1);
            int num_colors = 0;

            for (int i = 0; i < n_pairs; i++) {
                int pa = nearby_pairs[i].first;
                int pb = nearby_pairs[i].second;
                int min_color = std::max(particle_max_color[pa], particle_max_color[pb]) + 1;
                pair_color[i] = min_color;
                particle_max_color[pa] = std::max(particle_max_color[pa], min_color);
                particle_max_color[pb] = std::max(particle_max_color[pb], min_color);
                if (min_color >= num_colors) num_colors = min_color + 1;
            }

            // Build per-color batch indices
            std::vector<std::vector<int>> batches(num_colors);
            for (int i = 0; i < n_pairs; i++) {
                batches[pair_color[i]].push_back(i);
            }

            for (int n_collision = 0; n_collision < 100; n_collision++) {
                bool has_collision = false;
                int n_walls = nearby_walls.size();
                #pragma omp parallel for schedule(static) reduction(| : has_collision)
                for (int i = 0; i < n_walls; i++) {
                    int ind = nearby_walls[i];
                    if (particles[ind].resolve_wall_collision(w, h, rad)) {
                        has_collision |= true;
                    }
                }

                for (int c = 0; c < num_colors; c++) {
                    int batch_size = batches[c].size();
                    #pragma omp parallel for schedule(static) reduction(| : has_collision)
                    for (int bi = 0; bi < batch_size; bi++) {
                        auto& pair = nearby_pairs[batches[c][bi]];
                        auto& p1 = particles[pair.first];
                        auto& p2 = particles[pair.second];
                        if (resolve_collision(p1, p2)) {
                            has_collision |= true;
                        }
                    }
                }
                if (!has_collision) break;
            }
        #else
            for (int n_collision = 0; n_collision < 100; n_collision++) {
                bool has_collision = false;
                for (int i: nearby_walls) {
                    if (particles[i].resolve_wall_collision(w, h, rad)) {
                        has_collision |= true;
                    }
                }
                for (const auto& pair: nearby_pairs) {
                    auto& p1 = particles[pair.first];
                    auto& p2 = particles[pair.second];
                    if (resolve_collision(p1, p2)) {
                        has_collision |= true;
                    }
                }
            }
        #endif 
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
