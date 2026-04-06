#ifndef PARTICLEBOX_PARTICLE
#define PARTICLEBOX_PARTICLE

#include <cmath>
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif
struct Particle {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float rad = 1.0f;
    float mass = 1.0f;
    u_int8_t r = 255, g = 255, b = 255;

    bool is_overlap(const Particle& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float d = rad + other.rad;
        return dx * dx + dy * dy < d * d;
    }

    void step(float gravity) {
        vy += gravity;
        x += vx;
        y += vy;
    }

    bool resolve_wall_collision(float w, float h, float restitution = 1.0f) {
        bool resolved = false;
        if (x + rad > w) {
            x = w - rad;
            vx = -std::abs(vx) * restitution;
            resolved = true;
        } else if (x - rad < 0) {
            x = rad;
            vx = std::abs(vx) * restitution;
            resolved = true;
        }
        if (y + rad > h) {
            y = h - rad;
            vy = -std::abs(vy) * restitution;
            resolved = true;
        } else if (y - rad < 0) {
            y = rad;
            vy = std::abs(vy) * restitution;
            resolved = true;
        }
        return resolved;
    }

    friend bool resolve_collision(Particle& p1, Particle& p2, float restitution = 1.0f) {
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

        float total_mass = p1.mass + p2.mass;
        float scale = (1.0f + restitution) * dot / distance_sq;
        float s1 = p2.mass / total_mass * scale;
        float s2 = p1.mass / total_mass * scale;
        p1.vx += s1 * dx;
        p1.vy += s1 * dy;
        p2.vx -= s2 * dx;
        p2.vy -= s2 * dy;
        return true;
    }

    #ifdef _OPENMP
    friend bool resolve_collision(Particle& p1, Particle& p2, omp_lock_t& m1, omp_lock_t& m2, float restitution = 1.0f) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float distance_sq = dx * dx + dy * dy;
        if (distance_sq <= 1e-6f) return false;

        omp_set_lock(&m1);
        omp_set_lock(&m2);
        float dvx = p2.vx - p1.vx;
        float dvy = p2.vy - p1.vy;
        float dot = dvx * dx + dvy * dy;
        if (dot >= 0) {
            omp_unset_lock(&m2);
            omp_unset_lock(&m1);
            return false;
        }

        float total_mass = p1.mass + p2.mass;
        float scale = (1.0f + restitution) * dot / distance_sq;
        float s1 = p2.mass / total_mass * scale;
        float s2 = p1.mass / total_mass * scale;
        p1.vx += s1 * dx;
        p1.vy += s1 * dy;
        p2.vx -= s2 * dx;
        p2.vy -= s2 * dy;
        omp_unset_lock(&m2);
        omp_unset_lock(&m1);
        return true;
    }
    #endif

    friend std::ostream& operator<<(std::ostream& o, const Particle& p) {
        o << "[p: (" << p.x << "," << p.y << "), v: (" << p.vx << "," << p.vy << "), r: "
          << static_cast<int>(p.r) << "]";
        return o;
    }
};

#endif