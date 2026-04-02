#ifndef PARTICLEBOX_PARTICLE
#define PARTICLEBOX_PARTICLE
#include "vec.hpp"

template <typename T, size_t N>
requires Numeric<T>
struct Particle {
    Vec<T, N> pos;
    Vec<T, N> vel;
    T rad;

    Particle(const Vec<T, N> p, const Vec<T, N> v, T rad): pos(p), vel(v), rad(rad) {}

    template <size_t D = 2>
    bool is_overlap(const Particle<T, N>& other) {
        return pos.template dist<D>(other.pos) < pow<T, D>(rad + other.rad);
    }

    bool is_approaching(const Particle<T, N>& other) {
        Vec<T, N> p = other.pos - pos;
        Vec<T, N> v = other.vel - vel;
        return p.dot(v) < -0.0001f;
    }

    bool is_colliding(const Particle<T, N>& other) {
        return is_overlap(other) & is_approaching(other);
    }

    void step() {
        pos += vel;
    }

    friend std::ostream& operator<<(std::ostream& o, const Particle<T, N>& p) {
        o << "[p: " << p.pos << ", v: " << p.vel << ", r: " << p.rad << "]";
        return o;
    }

    friend void resolve_collision(Particle<T, N>& p1, Particle<T, N>& p2) {
        Vec<T, N> dp = p2.pos - p1.pos;
        Vec<T, N> dv = p2.vel - p1.vel;
        T dot_product = dv.dot(dp);
        if (dot_product >= 0) dot_product = 0;
        T collision_scale = dot_product / dp.template len<2>();
        Vec<T, N> delta = collision_scale * dp;
        p1.vel += delta;
        p2.vel -= delta;
    }
};

using Pf2 = Particle<float, 2>;

#endif