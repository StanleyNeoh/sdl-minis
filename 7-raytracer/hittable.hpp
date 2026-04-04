#ifndef RAYTRACER_HITTABLE
#define RAYTRACER_HITTABLE

#include "vec3.hpp"

struct Ray;

struct HitRecord {
    Vec3 p;
    Vec3 normal;
    float t;
    bool front_face;

    void set_face_normal(const Ray& ray, const Vec3& outward_normal);
};

struct Hittable {
    virtual bool hit(const Ray& ray, float ray_tmin, float ray_tmax, HitRecord& record) const = 0;
    virtual void color(Ray& ray) const = 0;
};

struct Sphere: Hittable {
    Vec3 center;
    float rad;

    Sphere(const Vec3& center, float rad): center(center), rad(rad) {}

    virtual bool hit(const Ray& ray, float ray_tmin, float ray_tmax, HitRecord& record) const override;
    virtual void color(Ray& ray) const override;
};

#endif