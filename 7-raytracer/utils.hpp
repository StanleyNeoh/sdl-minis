#ifndef RAYTRACER_UTILS
#define RAYTRACER_UTILS
#include <SDL.h>
#include <random>

struct Interval {
    float min_t;
    float max_t;

    bool contains(float t) const {
        return t >= min_t && t <= max_t;
    }

    float clamp(float t) const {
        if (t < min_t) return min_t;
        if (t > max_t) return max_t;
        return t;
    }
};

inline Uint32* offset(Uint32* ptr, int n_bytes) {
    return reinterpret_cast<Uint32*>(reinterpret_cast<Uint8*>(ptr) + n_bytes);
}

inline float random_float() {
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    static std::mt19937 gen;
    return dist(gen);
}

inline float random_float(float min, float max) {
    return min + (max - min) * random_float();
}

inline float lin_to_gamma(float lin) {
    if (lin > 0) return std::sqrt(lin);
    return 0;
}


#endif
