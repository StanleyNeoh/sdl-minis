#include "utils.hpp"
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