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

enum class MTMode { none, graph_coloring, mutex_locks, unsafe_no_lock, naive };

inline constexpr int NAIVE_MAX_PARTICLES = 10000;

template<typename QT>
struct ParticleBox {
    SDL_Texture* tex = nullptr;
    SDL_Texture* particle_tex = nullptr;
    int w = 1000;
    int h = 1000;
    float min_rad = 1.0f;
    float max_rad = 5.0f;
    float min_mass = 1.0f;
    float max_mass = 1.0f;
    QT quadtree;
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
                float prad = get_rand_float(min_rad, max_rad);
                float pmass = get_rand_float(min_mass, max_mass);
                // Hue from 240 (blue, light) → 0 (red, heavy), exponential decay on absolute mass
                RGB rgb = hue_to_rgb(std::exp(-pmass / 20.0f) * 240.0f);
                particles.emplace_back(Particle{.x = x, .y = y, .vx = vx, .vy = vy, .rad = prad, .mass = pmass, .r = rgb.r, .g = rgb.g, .b = rgb.b});
            }
        } else if (req < 0) {
            for (int i = 0; i < -req; i++) {
                particles.pop_back();
            }
        }
    }

    template<MTMode mode>
    void step(float gravity, float restitution) {
        int n = particles.size();
        for (int i = 0; i < n; i++) {
            particles[i].step(gravity);
        }

        std::vector<int> nearby_walls;
        std::vector<std::pair<int, int>> nearby_pairs;
        if constexpr (mode != MTMode::naive) {
            quadtree.reset(particles);
            quadtree.query_nearby_wall(max_rad, nearby_walls);
            quadtree.query_nearby_pairs(nearby_pairs);
        }

        if constexpr (mode == MTMode::none) {
            for (int n_collision = 0; n_collision < 100; n_collision++) {
                bool has_collision = false;
                for (int i: nearby_walls) {
                    if (particles[i].resolve_wall_collision(w, h, restitution)) {
                        has_collision |= true;
                    }
                }
                for (const auto& pair: nearby_pairs) {
                    auto& p1 = particles[pair.first];
                    auto& p2 = particles[pair.second];
                    if (p1.is_overlap(p2)) {
                        if (resolve_collision(p1, p2, restitution)) {
                            has_collision |= true;
                        }
                    }
                }
                if (!has_collision) break;
            }
        } else if constexpr (mode == MTMode::graph_coloring) {
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
                    if (particles[ind].resolve_wall_collision(w, h, restitution)) {
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
                        if (p1.is_overlap(p2)) {
                            if (resolve_collision(p1, p2, restitution)) {
                                has_collision |= true;
                            }
                        }
                    }
                }
                if (!has_collision) break;
            }
        } else if constexpr (mode == MTMode::mutex_locks) {
            #ifdef _OPENMP
            std::vector<omp_lock_t> locks(n);
            for (int i = 0; i < n; i++) omp_init_lock(&locks[i]);

            for (int n_collision = 0; n_collision < 100; n_collision++) {
                bool has_collision = false;
                int n_walls = nearby_walls.size();
                #pragma omp parallel for schedule(static) reduction(| : has_collision)
                for (int i = 0; i < n_walls; i++) {
                    int ind = nearby_walls[i];
                    if (particles[ind].resolve_wall_collision(w, h, restitution)) {
                        has_collision |= true;
                    }
                }

                int n_pairs_size = nearby_pairs.size();
                #pragma omp parallel for schedule(static) reduction(| : has_collision)
                for (int i = 0; i < n_pairs_size; i++) {
                    auto& pair = nearby_pairs[i];
                    auto& p1 = particles[pair.first];
                    auto& p2 = particles[pair.second];
                    if (p1.is_overlap(p2)) {
                        if (resolve_collision(p1, p2, locks[pair.first], locks[pair.second], restitution)) {
                            has_collision |= true;
                        }
                    }
                }
                if (!has_collision) break;
            }

            for (int i = 0; i < n; i++) omp_destroy_lock(&locks[i]);
            #endif
        } else if constexpr (mode == MTMode::unsafe_no_lock) {
            for (int n_collision = 0; n_collision < 100; n_collision++) {
                bool has_collision = false;
                int n_walls = nearby_walls.size();
                #pragma omp parallel for schedule(static) reduction(| : has_collision)
                for (int i = 0; i < n_walls; i++) {
                    int ind = nearby_walls[i];
                    if (particles[ind].resolve_wall_collision(w, h, restitution)) {
                        has_collision |= true;
                    }
                }

                int n_pairs_size = nearby_pairs.size();
                #pragma omp parallel for schedule(static) reduction(| : has_collision)
                for (int i = 0; i < n_pairs_size; i++) {
                    auto& pair = nearby_pairs[i];
                    auto& p1 = particles[pair.first];
                    auto& p2 = particles[pair.second];
                    if (p1.is_overlap(p2)) {
                        if (resolve_collision(p1, p2, restitution)) {
                            has_collision |= true;
                        }
                    }
                }
                if (!has_collision) break;
            }
        } else if constexpr (mode == MTMode::naive) {
            for (int n_collision = 0; n_collision < 100; n_collision++) {
                bool has_collision = false;
                for (int i = 0; i < n; i++) {
                    if (particles[i].resolve_wall_collision(w, h, restitution)) {
                        has_collision |= true;
                    }
                }
                for (int i = 0; i < n; i++) {
                    for (int j = i + 1; j < n; j++) {
                        if (particles[i].is_overlap(particles[j])) {
                            if (resolve_collision(particles[i], particles[j], restitution)) {
                                has_collision |= true;
                            }
                        }
                    }
                }
                if (!has_collision) break;
            }
        }
    }

    void step(MTMode mode, float gravity = 0.0f, float restitution = 1.0f) {
        switch (mode) {
            case MTMode::naive: step<MTMode::naive>(gravity, restitution); break;
            case MTMode::none: step<MTMode::none>(gravity, restitution); break;
        #ifdef _OPENMP
            case MTMode::graph_coloring: step<MTMode::graph_coloring>(gravity, restitution); break;
            case MTMode::mutex_locks: step<MTMode::mutex_locks>(gravity, restitution); break;
            case MTMode::unsafe_no_lock: step<MTMode::unsafe_no_lock>(gravity, restitution); break;
        #endif
            default: step<MTMode::none>(gravity, restitution); break;
        }
    }

    SDL_Texture* render(SDL_Renderer* renderer) {
        SDL_SetRenderTarget(renderer, tex);
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        for (auto& p : particles) {
            SDL_SetTextureColorMod(particle_tex, p.r, p.g, p.b);
            SDL_FRect rect{p.x - p.rad, p.y - p.rad, p.rad * 2.0f, p.rad * 2.0f};
            SDL_RenderCopyF(renderer, particle_tex, nullptr, &rect);
        }
        SDL_SetRenderTarget(renderer, nullptr);
        return tex;
    }
};

#endif
