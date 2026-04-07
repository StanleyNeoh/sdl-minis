#ifndef RAYTRACER_RAY
#define RAYTRACER_RAY

#include <vector>
#include "vec3.hpp"
#include "utils.hpp"
#include <SDL.h>

struct Ray {
    Vec3 orig = {0, 0, 0};
    Vec3 dir = {0, 0, 0};

    Vec3 at(float t) const {
        return orig + t * dir;
    }

    struct CutPlaneSpanRes {
        float ca;
        float cb;
        float t;
        Vec3 normal; // a x b;
    };

    bool cutPlaneSpan(const Vec3& x, const Vec3& a, const Vec3& b, CutPlaneSpanRes& res, float eps = 1e-6) const {
        Vec3 y = x - orig;
        Vec3 yd = y.cross(dir);
        Vec3 ab = a.cross(b);
        float ab_d = a.cross(b).dot(dir);
        if (abs(ab_d) < eps) return false;
        res.ca = yd.dot(b) / ab_d;
        res.cb = yd.dot(a) / -ab_d;
        res.t = y.cross(a).dot(b) / ab_d;
        res.normal = ab.unit();
        return true;
    }
    struct CutPlaneNormalRes {
        float t;
    };

    bool cutPlaneNormal(const Vec3& x, const Vec3& normal, CutPlaneNormalRes& res, float eps = 1e-6) const {
        Vec3 y = x - orig;
        float d_n = dir.dot(normal);
        if (abs(d_n) < eps) return false;
        res.t = y.dot(normal) / d_n;
        return true;
    }
};

#endif