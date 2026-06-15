#ifndef UTILS_HPP
#define UTILS_HPP

#include <chrono>
#include <thread>
#include <random>
#include <cmath>
#include <iostream>
#include <array>

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

struct Vec2 {
    float x = 0;
    float y = 0;


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

    Vec2 vec2_real(float length = 1) {
        float d = Rand::real(2 * M_PI, 0);
        return Vec2(std::sin(d) * length, std::cos(d) * length);
    }

    template <int N = 63, int M = 63>
    struct Perlin2D {
        int width;
        int height;
        static constexpr int n_vertices = (N + 1) * (M + 1);
        std::array<Vec2, n_vertices> board;

        float ch;
        float cw;

        Perlin2D(int width, int height): 
            width(width), 
            height(height),
            cw(static_cast<float>(width) / M),
            ch(static_cast<float>(height) / N)
        {
            reset();
        }

        void reset() {
            for (int i = 0; i < n_vertices; i++) {
                board[i] = vec2_real();
            }
        }

        float fade(float x) {
            // x should be in [0, 1]
            return x * x * x * (6 * x * x - 15 * x + 10);
        }

        float lerp(float t, float x, float y) {
            return x + t * (y - x);
        }

        float query(int x, int y) {
            float fr = static_cast<float>(y) / ch;
            float fc = static_cast<float>(x) / cw;
            int r = fr; // int component
            int c = fc; // int component
            fr -= r; // frac component
            fc -= c; // frac component

            Vec2& v00 = board[r * (M + 1) + c];
            Vec2& v01 = board[r * (M + 1) + c + 1];
            Vec2& v10 = board[(r + 1) * (M + 1) + c];
            Vec2& v11 = board[(r + 1) * (M + 1) + c + 1];
            float u = fade(fr);
            float v = fade(fc);
            float dx = fc * cw;
            float dy = fr * ch;
            return lerp(u,
                lerp(v, 
                    v00.x * dx + v00.y * dy, 
                    v01.x * (1.0 - dx) + v01.y * dy
                ),
                lerp(v, 
                    v10.x * dx + v10.y * (1.0 - dy), 
                    v11.x * (1.0 - dx) + v11.y * (1.0 - dy)
                )
            );
        }
    };
};

#endif