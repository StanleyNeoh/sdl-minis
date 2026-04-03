#ifndef PARTICLEBOX_PARTICLEBOX
#define PARTICLEBOX_PARTICLEBOX

#include <vector>
#include <list>
#include <iterator>
#include <array>
#include <memory>
#include "particle.hpp"

struct ParticleBox {
    std::vector<Pf2> particles;

    void add_particle(const Pf2& particle) {
        particles.push_back(particle);
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
                for (int j = i+1; j < n; j++) {
                    auto& p1 = particles[i];
                    auto& p2 = particles[j];
                    if (p1.is_colliding(p2)) {
                        no_collision = false;
                        resolve_collision(p1, p2);
                    }
                }
            }
        }
    }
};

#endif