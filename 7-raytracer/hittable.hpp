#ifndef RAYTRACER_HITTABLE
#define RAYTRACER_HITTABLE

#include "vec3.hpp"
#include "utils.hpp"
#include <vector>
#include <iostream>

struct Ray;

struct HitRecord {
    Vec3 p;
    Vec3 normal;
    float t;
    bool front_face;

    void set_face_normal(const Ray& ray, const Vec3& outward_normal);
};

struct Hittable {
    virtual bool hit(Ray& ray, Interval trange, HitRecord& record) const = 0;
};

struct Hittables: Hittable {
    std::vector<Hittable*> hittables;

    void add(Hittable* hittable) {
        hittables.push_back(hittable);
    }

    virtual bool hit(Ray& ray, Interval trange, HitRecord& record) const override;
};

struct Sphere: Hittable {
    Vec3 center;
    float rad;

    Sphere(const Vec3& center, float rad): center(center), rad(rad) {}

    virtual bool hit(Ray& ray, Interval trange, HitRecord& record) const override;
};

struct Cube: Hittable {
    Vec3 pos;
    Vec3 dim;

    Cube(const Vec3& pos, const Vec3& dim): pos(pos), dim(dim) {}

    virtual bool hit(Ray& ray, Interval trange, HitRecord& record) const override;
};

struct Plane: Hittable {
    Vec3 normal;
    Vec3 pos;

    Plane(const Vec3& normal, const Vec3& pos): normal(normal), pos(pos) {}

    virtual bool hit(Ray& ray, Interval trange, HitRecord& record) const override;
};


#endif