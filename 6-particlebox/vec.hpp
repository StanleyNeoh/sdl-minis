#ifndef PARTICLEBOX_VEC
#define PARTICLEBOX_VEC

#include <iostream>
#include <cmath>
#include <concepts>

template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <typename T, size_t p>
requires Numeric<T>
constexpr T pow(T a) {
    if constexpr (p == 0) {
        return 1;
    } else {
        return a * pow<T, p-1>(a);
    }
}

template <typename T = float, size_t N = 2>
requires Numeric<T>
struct Vec: std::array<T, N> {
    [[nodiscard]] constexpr Vec<T, N> operator-() const {
        Vec<T, N> ret;
        for (size_t i = 0; i < N; i++) {
            ret[i] = -(*this)[i];
        }
        return ret;
    }

    template <size_t D = 2>
    [[nodiscard]] constexpr T len() const {
        T ret = 0;
        for (size_t i = 0; i < N; i++) {
            ret += pow<T, D>((*this)[i]);
        }
        return ret;
    }

    template <size_t D = 2>
    [[nodiscard]] constexpr T dist(const Vec<T, N>& other) const {
        T ret = 0;
        for (size_t i = 0; i < N; i++) {
            ret += pow<T, D>((*this)[i] - other[i]);
        }
        return ret;
    }

    [[nodiscard]] constexpr T dot(const Vec<T, N>& other) const {
        T ret = 0;
        for (size_t i = 0; i < N; i++) {
            ret += (*this)[i] * other[i];
        }
        return ret;
    }

    friend std::ostream& operator<<(std::ostream& o, const Vec<T, N>& vec) {
        o << '(';
        for (size_t i = 0; i < N; i++) {
            o << vec[i];
            if (i+1 < N) o << ',';
        }
        o << ')';
        return o;
    }

    constexpr Vec<T,N>& operator+=(const Vec<T,N>& other) {
        for (size_t i = 0; i < N; i++) {
            (*this)[i] += other[i];
        }
        return *this;
    }

    constexpr Vec<T,N>& operator-=(const Vec<T,N>& other) {
        for (size_t i = 0; i < N; i++) {
            (*this)[i] -= other[i];
        }
        return *this;
    }

    [[nodiscard]] friend constexpr Vec<T, N> operator+(const Vec<T, N>& v1, const Vec<T, N>& v2) {
        Vec<T, N> ret;
        for (size_t i = 0; i < N; i++) {
            ret[i] = v1[i] + v2[i];
        }
        return ret;
    }

    [[nodiscard]] friend constexpr Vec<T, N> operator-(const Vec<T, N>& v1, const Vec<T, N>& v2) {
        Vec<T, N> ret;
        for (size_t i = 0; i < N; i++) {
            ret[i] = v1[i] - v2[i];
        }
        return ret;
    }

    [[nodiscard]] friend constexpr Vec<T, N> operator*(const Vec<T, N>& v1, const Vec<T, N>& v2) {
        Vec<T, N> ret;
        for (size_t i = 0; i < N; i++) {
            ret[i] = v1[i] * v2[i];
        }
        return ret;
    }

    [[nodiscard]] friend constexpr Vec<T, N> operator*(T k, const Vec<T, N>& v1) {
        Vec<T, N> ret;
        for (size_t i = 0; i < N; i++) {
            ret[i] = k * v1[i];
        }
        return ret;
    }

    [[nodiscard]] friend constexpr Vec<T, N> operator*(const Vec<T, N>& v1, T k) {
        return k * v1;
    }
};

using Vf2 = Vec<float, 2>;

#endif