#ifndef COMMON_UTILS_HPP
#define COMMON_UTILS_HPP

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

    inline int integer(int max, int min = 0) {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(engine);
    }

    inline float real(float max = 1, float min = 0) {
        std::uniform_real_distribution<float> distribution(min, max);
        return distribution(engine);
    }

    inline Vec2 vec2_real(float length = 1) {
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
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (x >= width) x = width - 1;
            if (y >= height) y = height - 1;

            float gx = static_cast<float>(x) / cw;
            float gy = static_cast<float>(y) / ch;

            int c = static_cast<int>(gx);
            int r = static_cast<int>(gy);
            if (c >= M) c = M - 1;
            if (r >= N) r = N - 1;

            float xf = gx - c;
            float yf = gy - r;

            const Vec2& g00 = board[r * (M + 1) + c];
            const Vec2& g10 = board[r * (M + 1) + c + 1];
            const Vec2& g01 = board[(r + 1) * (M + 1) + c];
            const Vec2& g11 = board[(r + 1) * (M + 1) + c + 1];

            float n00 = g00.x * xf + g00.y * yf;
            float n10 = g10.x * (xf - 1.0f) + g10.y * yf;
            float n01 = g01.x * xf + g01.y * (yf - 1.0f);
            float n11 = g11.x * (xf - 1.0f) + g11.y * (yf - 1.0f);

            float u = fade(xf);
            float v = fade(yf);
            float nx0 = lerp(u, n00, n10);
            float nx1 = lerp(u, n01, n11);
            return lerp(v, nx0, nx1);
        }
    };
};

#endif