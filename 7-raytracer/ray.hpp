#ifndef RAYTRACER_RAY
#define RAYTRACER_RAY

#include <vector>
#include "hittable.hpp"
#include "vec3.hpp"
#include <SDL.h>

struct Ray {
    Vec3 orig;
    Vec3 dir;
    Vec3 color = {0, 0, 0};

    Vec3 at(float t) const;
    void cast(const std::vector<Hittable*>& hittables);

    struct CutPlaneSpanRes {
        float ca;
        float cb;
        float t;
        Vec3 pos;
        Vec3 normal; // a x b
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
        res.pos = at(res.t);
        res.normal = ab.unit();
        return true;
    }
    struct CutPlaneNormalRes {
        float t;
        Vec3 pos;
    };

    bool cutPlaneNormal(const Vec3& x, const Vec3& normal, CutPlaneNormalRes& res, float eps = 1e-6) const {
        Vec3 y = x - orig;
        float d_n = dir.dot(normal);
        if (abs(d_n) < eps) return false;
        res.t = y.dot(normal) / d_n;
        res.pos = at(res.t);
        return true;
    }

    Uint32 argb_color();
};

#endif