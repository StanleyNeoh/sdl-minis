#include "utils.hpp"
#include <cmath>
#include <cstdint>
#include <random>

int get_rand_int(int l, int r) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> uniform_dist(l, r);
    return uniform_dist(gen);
}

float get_rand_float(float l, float r) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> uniform_dist(l, r);
    return uniform_dist(gen);
}

uint32_t get_rand_color() {
    return 0xFF000000 | (get_rand_int(50, 255) << 16) | (get_rand_int(50, 255) << 8) | (get_rand_int(50, 255));
}

RGB hue_to_rgb(float hue) {
    float c = 1.0f;
    float hp = hue / 60.0f;
    float x = c * (1.0f - std::abs(std::fmod(hp, 2.0f) - 1.0f));
    float r, g, b;
    if      (hp < 1) { r=c; g=x; b=0; }
    else if (hp < 2) { r=x; g=c; b=0; }
    else if (hp < 3) { r=0; g=c; b=x; }
    else if (hp < 4) { r=0; g=x; b=c; }
    else             { r=x; g=0; b=c; }
    return { static_cast<uint8_t>(r * 255), static_cast<uint8_t>(g * 255), static_cast<uint8_t>(b * 255) };
}