#ifndef RAYTRACER_VEC3
#define RAYTRACER_VEC3

#include <cmath>
#include <iostream>

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

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

    Vec3 yaw(float rad) { // about z
        float c = std::cos(rad);
        float s = std::sin(rad);
        return {
            c * x - s * y,
            s * x + c * y,
            z
        };
    }

    Vec3 pitch(float rad) { // about y
        float c = std::cos(rad);
        float s = std::sin(rad);
        return {
            c * x + s * z,
            y,
            -s * x + c * z
        };
    }

    Vec3 roll(float rad) { // about x
        float c = std::cos(rad);
        float s = std::sin(rad);
        return {
            x, 
            c * y - s * z,
            s * y + c * z
        };
    }

    Vec3 rot(float rad, const Vec3& k) {
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


    friend Vec3 operator+(const Vec3& a, const Vec3& b) {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    friend Vec3 operator-(const Vec3& a, const Vec3& b) {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
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