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

    struct CutRayRes {
        float dist;
        float t;
        Vec3 normal;
    };

    bool cutRay(const Ray& other, float thickness, CutRayRes& res) const {
        Vec3 w = orig - other.orig;
        float a = dir.dot(dir);
        float b = dir.dot(other.dir);
        float c = other.dir.dot(other.dir);
        float d = dir.dot(w);
        float e = other.dir.dot(w);
        float D = a*c - b*b;

        float tc;
        if (D < 1e-6f) {
            res.t = 0;
            tc = (b > c ? d / b : e / c);
            if (tc < 0.0f) {
                tc = 0.0f;
                res.t = -d / a;
            } else if (tc > 1.0f) {
                tc = 1.0f;
                res.t = (b - d) / a;
            }
        } else {
            res.t = (b*e - c*d) / D;
            tc = (a*e - b*d) / D;
            if (tc < 0.0f) {
                tc = 0.0f;
                res.t = -d / a;
            } else if (tc > 1.0f) {
                tc = 1.0f;
                res.t = (b - d) / a;
            }
        }

        Vec3 closest_pt1 = orig + res.t * dir;
        Vec3 closest_pt2 = other.orig + tc * other.dir;
        Vec3 diff = closest_pt1 - closest_pt2;
        res.dist = diff.len();
        res.normal = diff.near_zero() ? Vec3{0, 1, 0} : diff.unit();
        return res.dist <= thickness;
    }
};

#endif