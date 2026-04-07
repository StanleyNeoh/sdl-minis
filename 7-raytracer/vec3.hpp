#ifndef RAYTRACER_VEC3
#define RAYTRACER_VEC3

#include <cmath>
#include <iostream>
#include "utils.hpp"

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    static Vec3 random_unit() {
        float u = random_float(0.0f, 2 * M_PI);
        float v = random_float(0.0f, 2 * M_PI);
        return Vec3{
            std::sin(v) * std::sin(u),
            std::sin(v) * std::cos(u),
            std::cos(v)
        };
    }

    float dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vec3 cross(const Vec3& other) const {
        return {
            y * other.z - other.y * z,
            z * other.x - other.z * x,
            x * other.y - other.x * y
        };
    }

    Vec3 yaw(float rad) const { // about z
        float c = std::cos(rad);
        float s = std::sin(rad);
        return {
            c * x - s * y,
            s * x + c * y,
            z
        };
    }

    Vec3 pitch(float rad) const { // about y
        float c = std::cos(rad);
        float s = std::sin(rad);
        return {
            c * x + s * z,
            y,
            -s * x + c * z
        };
    }

    Vec3 roll(float rad) const { // about x
        float c = std::cos(rad);
        float s = std::sin(rad);
        return {
            x, 
            c * y - s * z,
            s * y + c * z
        };
    }

    Vec3 rot(float rad, const Vec3& k) const {
        float c = std::cos(rad);
        float s = std::sin(rad);
        const auto& v = *this;
        return (c * v) + (s * k.cross(v)) + ((1 - c) * k.dot(v)) * k;
    }

    float len2() const {
        return x * x + y * y + z * z;
    }

    float len() const {
        return std::sqrt(len2());
    }

    Vec3 unit() const {
        return *this / len();
    }

    void normalize() {
        float f = len();
        x /= f;
        y /= f;
        z /= f;
    }

    Uint32 as_argb() const {
        static const Interval interval(0.000, 0.9999);
        int a = 255;
        int r = 256 * interval.clamp(lin_to_gamma(x));
        int g = 256 * interval.clamp(lin_to_gamma(y));
        int b = 256 * interval.clamp(lin_to_gamma(z));
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    bool near_zero(float eps=1e-6) const {
        return abs(x) < eps && abs(y) < eps && abs(z) < eps;
    }

    Vec3 reflect(const Vec3& n) const {
        return *this - 2 * dot(n) * n;
    }

    Vec3 refract(const Vec3& normal, float rel_ri) const { 
        // rel_ri = n_2 / n_1
        float cos_theta = std::min(-normal.dot(*this), 1.0f);
        Vec3 r_out_perp = (*this + cos_theta * normal) / rel_ri;
        Vec3 r_out_par = -std::sqrt(std::abs(1.0 - r_out_perp.len2())) * normal;
        return r_out_par + r_out_perp;
    }

    Vec3 operator-() const {
        return {-x, -y, -z};
    }

    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3& operator-=(const Vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vec3& operator/=(float k) {
        x /= k;
        y /= k;
        z /= k;
        return *this;
    }

    bool operator<(const Vec3& other) const {
        return (
            x < other.x &&
            y < other.y &&
            z < other.z        
        );
    }

    bool operator>(const Vec3& other) const {
        return other < (*this);
    }

    friend Vec3 operator+(const Vec3& a, const Vec3& b) {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    friend Vec3 operator-(const Vec3& a, const Vec3& b) {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    friend Vec3 operator*(const Vec3& a, const Vec3& b) {
        return {a.x * b.x, a.y * b.y, a.z * b.z};
    }

    friend Vec3 operator*(float k, const Vec3& b) {
        return {k * b.x, k * b.y, k * b.z};
    }

    friend Vec3 operator*(const Vec3& b, float k) {
        return k * b;
    }

    friend Vec3 operator/(const Vec3& a, float k) {
        return (1/k) * a;
    }

    friend std::ostream& operator<<(std::ostream& o, const Vec3& v) {
        o << "(" << v.x << "," << v.y << "," << v.z << ")";
        return o;
    }
};

#endif