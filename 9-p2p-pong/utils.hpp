#ifndef UTILS_HPP
#define UTILS_HPP

#include <chrono>
#include <thread>
#include <random>
#include <cmath>
#include <iostream>

inline std::time_t curr_time() {
    using Clock = std::chrono::system_clock;
    return Clock::to_time_t(Clock::now());
}

struct Backoff {
    size_t spinCount = 0;

    void backoff() {
        if (++spinCount < 64) {
            std::this_thread::yield();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            spinCount = 0;
        }
    }

    void reset() {
        spinCount = 0;
    }
};

namespace Rand {
    static std::mt19937 engine(std::random_device{}());

    int integer(int max, int min = 0) {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(engine);
    }

    float real(float max = 1, float min = 0) {
        std::uniform_real_distribution<float> distribution(min, max);
        return distribution(engine);
    }
};

struct Vec2 {
    float x = 0;
    float y = 0;

    static Vec2 rand(float length = 1) {
        float d = Rand::real(2 * M_PI, 0);
        return Vec2(std::sin(d) * length, std::cos(d) * length);
    }

    Vec2() = default;
    Vec2(float x, float y): x(x), y(y) {}

    float l2() const {
        return x * x + y * y;
    }

    float l() const {
        return std::sqrt(l2());
    }

    void normalise() {
        float len = l();
        x /= len;
        y /= len;
    }

    Vec2 unit() const {
        Vec2 v(x, y);
        v.normalise();
        return v;
    }

    float dot(const Vec2& other) const {
        return x * other.x + y * other.y;
    }

    friend std::ostream& operator<<(std::ostream& o, const Vec2& vec) {
        o << '(' << vec.x << ',' << vec.y << ')';
        return o;
    }
};


#endif