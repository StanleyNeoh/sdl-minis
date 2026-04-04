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

    bool resolve_wall_collision(float w, float h) {
        bool resolved = false;
        if (x + rad > w) {
            x = w - rad;
            vx = -std::abs(vx);
            resolved = true;
        } else if (x - rad < 0) {
            x = rad;
            vx = std::abs(vx);
            resolved = true;
        }
        if (y + rad > h) {
            y = h - rad;
            vy = -std::abs(vy);
            resolved = true;
        } else if (y - rad < 0) {
            y = rad;
            vy = std::abs(vy);
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

    friend bool resolve_collision(Particle& p1, Particle& p2, omp_lock_t& m1, omp_lock_t& m2) {
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

        float scale = dot / distance_sq;
        p1.vx += scale * dx;
        p1.vy += scale * dy;
        p2.vx -= scale * dx;
        p2.vy -= scale * dy;
        omp_unset_lock(&m2);
        omp_unset_lock(&m1);
        return true;
    }

    friend std::ostream& operator<<(std::ostream& o, const Particle& p) {
        o << "[p: (" << p.x << "," << p.y << "), v: (" << p.vx << "," << p.vy << "), r: "
          << static_cast<int>(p.r) << "]";
        return o;
    }
};

#endif