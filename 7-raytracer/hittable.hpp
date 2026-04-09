#ifndef RAYTRACER_HITTABLE
#define RAYTRACER_HITTABLE

#include "vec3.hpp"
#include "ray.hpp"
#include "utils.hpp"
#include "material.hpp"
#include <vector>
#include <iostream>

struct Hittable {
    Material* mat = nullptr;

    Hittable(Material* mat): mat(mat) {}

    virtual bool hit(const Ray& ray, const Interval& trange, HitRecord& record) const = 0;
};

struct Hittables {
    std::vector<Hittable*> hittables;
    Material* bgMat;

    Hittables(Material* bgMat): bgMat(bgMat) {}

    void add(Hittable* hittable) {
        hittables.push_back(hittable);
    }

    bool hit(const Ray& ray, Interval search_range, HitRecord& record) const {
        bool has_hit = false;
        for (Hittable* ptr: hittables) {
            if (ptr->hit(ray, search_range, record)) {
                search_range.max_t = record.t;
                has_hit = true;
            }
        }
        return has_hit;
    }

    Vec3 get_color(const Ray& ray, const Interval& trange, int jumps_left) const {
        if (jumps_left <= 0) {
            return {0, 0, 0};
        }

        HitRecord record;
        bool has_hit = hit(ray, trange, record);
        if (!has_hit) record.set(ray, bgMat);
        
        Vec3 attenuation;
        Ray scatter;
        if (record.scatter(attenuation, scatter)) {
            return attenuation * get_color(scatter, trange, jumps_left-1);
        };
        return attenuation;
    }
};

struct Sphere: Hittable {
    Ray center;
    float rad;

    Sphere(const Ray& center, float rad, Material* mat): Hittable(mat), center(center), rad(rad) {}

    bool hit(const Ray& ray, const Interval& trange, HitRecord& record) const override {
        Vec3 center = this->center.at(ray.time);
        Vec3 oc = center - ray.orig;
        float a = ray.dir.len2();
        float h = oc.dot(ray.dir);
        float c = oc.len2() - rad * rad;
        float discriminant = h*h - a*c;
        if (discriminant < 0) {
            return false;
        } 
        float sqrtd = std::sqrt(discriminant);
        auto root = (h - sqrtd) / a;
        if (!trange.contains(root)) {
            root = (h + sqrtd) / a;
            if (!trange.contains(root)) {
                return false;
            }
        }
        record.set(ray, mat, root, (ray.at(root) - center) / rad);
        return true;
    }
};

struct Cube: Hittable {
    Ray pos;
    Vec3 dim;

    Cube(const Ray& pos, const Vec3& dim, Material* mat): Hittable(mat), pos(pos), dim(dim) {}

    bool hit(const Ray& ray, const Interval& trange, HitRecord& record) const {
        Vec3 pos = this->pos.at(ray.time);
        Vec3 opp = pos + dim;
        int count = 0;
        Ray::CutPlaneSpanRes hits[2];
        Ray::CutPlaneSpanRes res;
        auto check = [&](const Ray::CutPlaneSpanRes& _res) {
            if (_res.ca <= 1 && _res.ca >= 0 && _res.cb <= 1 && _res.cb >= 0) {
                if (count < 2) hits[count] = _res;
                count++;
            }
        };
        if (ray.cutPlaneSpan(pos, {0, dim.y, 0},  {dim.x, 0, 0}, res)) check(res);
        if (ray.cutPlaneSpan(pos, {dim.x, 0, 0}, {0, 0, dim.z}, res)) check(res);
        if (ray.cutPlaneSpan(pos, {0, 0, dim.z}, {0, dim.y, 0}, res)) check(res);
        if (ray.cutPlaneSpan(opp, {-dim.x, 0, 0}, {0, -dim.y, 0}, res)) check(res);
        if (ray.cutPlaneSpan(opp, {0, 0, -dim.z}, {-dim.x, 0, 0}, res)) check(res);
        if (ray.cutPlaneSpan(opp, {0, -dim.y, 0}, {0, 0, -dim.z}, res)) check(res);
        if (count != 2) return false;
        if (hits[0].t > hits[1].t) std::swap(hits[0], hits[1]);
        for (int i = 0; i < 2; i++) {
            if (!trange.contains(hits[i].t)) continue;
            record.set(ray, mat, hits[i].t, hits[i].normal);
            return true;
        }
        return false;
    }
};

struct Plane: Hittable {
    Vec3 normal;
    Vec3 pos;

    Plane(const Vec3& normal, const Vec3& pos, Material* mat): Hittable(mat), normal(normal), pos(pos) {}

    bool hit(const Ray& ray, const Interval& trange, HitRecord& record) const {
        int count = 0;
        Ray::CutPlaneNormalRes res;
        if (!ray.cutPlaneNormal(pos, normal, res)) return false;
        if (!trange.contains(res.t)) return false;
        record.set(ray, mat, res.t, normal);
        return true;
    }
};

struct WireFrame: Hittable {
    Ray pos;
    Vec3 dim;
    float thickness = 0.01;

    WireFrame(const Ray& pos, const Vec3& dim, Material* mat): Hittable(mat), pos(pos), dim(dim) {}

    bool hit(const Ray& ray, const Interval& trange, HitRecord& record) const {
        Vec3 pos = this->pos.at(ray.time);
        bool has_hit = false;
        float closest_so_far = FLT_MAX;
        auto check = [&](Ray::CutRayRes& res) {
            if (res.t < closest_so_far && trange.contains(res.t)) {
                record.set(ray, mat, res.t, res.normal);
                closest_so_far = res.t;
                has_hit = true;
            }
        };

        Ray::CutRayRes res;
        if (ray.cutRay(Ray(pos, {dim.x, 0, 0}), thickness, res)) check(res);
        if (ray.cutRay(Ray(pos, {0, dim.y, 0}), thickness, res)) check(res);
        if (ray.cutRay(Ray(pos, {0, 0, dim.z}), thickness, res)) check(res);
        Vec3 pos1 = pos + Vec3{dim.x, dim.y, 0};
        if (ray.cutRay(Ray(pos1, {-dim.x, 0, 0}), thickness, res)) check(res);
        if (ray.cutRay(Ray(pos1, {0, -dim.y, 0}), thickness, res)) check(res);
        if (ray.cutRay(Ray(pos1, {0, 0, dim.z}), thickness, res)) check(res);
        Vec3 pos2 = pos + Vec3{0, dim.y, dim.z};
        if (ray.cutRay(Ray(pos2, {dim.x, 0, 0}), thickness, res)) check(res);
        if (ray.cutRay(Ray(pos2, {0, -dim.y, 0}), thickness, res)) check(res);
        if (ray.cutRay(Ray(pos2, {0, 0, -dim.z}), thickness, res)) check(res);
        Vec3 pos3 = pos + Vec3{dim.x, 0, dim.z};
        if (ray.cutRay(Ray(pos3, {-dim.x, 0, 0}), thickness, res)) check(res);
        if (ray.cutRay(Ray(pos3, {0, dim.y, 0}), thickness, res)) check(res);
        if (ray.cutRay(Ray(pos3, {0, 0, -dim.z}), thickness, res)) check(res);
        return has_hit;
    }
};


#endif