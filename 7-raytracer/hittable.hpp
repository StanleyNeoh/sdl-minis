#ifndef RAYTRACER_HITTABLE
#define RAYTRACER_HITTABLE

#include "vec3.hpp"
#include <iostream>

struct Ray;

struct Hittable {
    struct HitRecord {
        Vec3 p;
        Vec3 normal;
        float t;
        bool front_face;

        void set_face_normal(const Ray& ray, const Vec3& outward_normal);
    };

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

struct Cube: Hittable {
    Vec3 pos;
    Vec3 dim;

    Cube(const Vec3& pos, const Vec3& dim): pos(pos), dim(dim) {}

    bool contains(Vec3& _pos) {
        return (
            pos < _pos && 
            _pos < pos + dim
        );
    }

    virtual bool hit(const Ray& ray, float ray_tmin, float ray_tmax, HitRecord& record) const override;
    virtual void color(Ray& ray) const override;
};

struct Plane: Hittable {
    Vec3 normal;
    Vec3 pos;

    Plane(const Vec3& normal, const Vec3& pos): normal(normal), pos(pos) {}

    virtual bool hit(const Ray& ray, float ray_tmin, float ray_tmax, HitRecord& record) const override;
    virtual void color(Ray& ray) const override;
};


#endif