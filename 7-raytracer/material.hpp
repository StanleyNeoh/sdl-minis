#ifndef RAYTRACER_MATERIAL
#define RAYTRACER_MATERIAL

#include "float.h"
#include "vec3.hpp"

struct HitRecord;
struct Material {
    virtual bool scatter(const HitRecord& record, Vec3& attenuation, Ray& scattered) const = 0;
};

struct HitRecord {
    float t = 0;
    Vec3 pos = {0, 0, 0};
    Vec3 dir = {0, 0, 0};
    Material* mat = nullptr;
    bool front_face = true;
    Vec3 normal = {0, 0, 0};

    void set(const Ray& ray, Material* mat) {
        this->t = FLT_MAX;
        this->pos = {0, 0, 0};
        this->dir = ray.dir.unit();
        this->mat = mat;
        this->front_face = true;
        this->normal = {0, 0, 0};
    }

    void set(const Ray& ray, Material* mat, float t, const Vec3& outward_normal) {
        this->t = t;
        this->pos = ray.at(t);
        this->dir = ray.dir.unit();
        this->mat = mat;
        this->front_face = ray.dir.dot(outward_normal) < 0;
        this->normal = this->front_face ? outward_normal.unit() : -outward_normal.unit();
    }

    bool scatter(Vec3& attenuation, Ray& scattered) {
        if (mat == nullptr) {
            attenuation = {0, 0, 0};
            return false;
        }
        return mat->scatter(*this, attenuation, scattered);
    }
};

struct UniformGlow: Material {
    Vec3 color;

    UniformGlow(const Vec3& color): color(color) {}

    virtual bool scatter(const HitRecord& record, Vec3& attenuation, Ray& scattered) const override {
        attenuation = color;
        return false;
    }
};

struct SurfaceNormalGlow: Material {
    virtual bool scatter(const HitRecord& record, Vec3& attenuation, Ray& scattered) const override {
        attenuation = 0.5 * (record.normal + Vec3{1,1,1});
        return false;
    }
};

struct GradientGlow: Material {
    Vec3 dir;
    Vec3 start_color;
    Vec3 end_color;

    GradientGlow(const Vec3& dir, const Vec3& start_color, const Vec3& end_color): dir(dir), start_color(start_color), end_color(end_color) {}

    virtual bool scatter(const HitRecord& record, Vec3& attenuation, Ray& scattered) const override {
        float a = 0.5 * (record.dir.dot(dir) + 1.0);
        attenuation = (1.0 - a) * start_color + a * end_color;
        return false;
    }
};

struct Lambertian: Material {
    Vec3 albedo;

    Lambertian(const Vec3& albedo): albedo(albedo) {}

    virtual bool scatter(const HitRecord& record, Vec3& attenuation, Ray& scattered) const override {
        Vec3 scattered_dir = record.normal + Vec3::random_unit();
        if (scattered_dir.near_zero()) scattered_dir = record.normal;
        scattered = Ray(record.pos, scattered_dir);
        attenuation = albedo;
        return true;
    }
};

struct Metal: Material {
    Vec3 albedo;
    float fuzz;

    Metal(const Vec3& albedo, float fuzz): albedo(albedo), fuzz(fuzz) {}

    bool scatter(const HitRecord& record, Vec3& attenuation, Ray& scattered) const override {
        Vec3 reflected = record.dir.reflect(record.normal);
        reflected += fuzz * Vec3::random_unit();
        scattered = Ray(record.pos, reflected);
        attenuation = albedo;
        return reflected.dot(record.normal) > 0;
    }
};

struct Dielectric: Material {
    Vec3 color;
    float ri;

    Dielectric(Vec3 color, float ri): color(color), ri(ri) {}

    static float reflectance(float cosine, float ref_idx) {
        float r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
        r0 = r0 * r0;
        return r0 + (1.0f - r0) * std::pow((1.0f - cosine), 5.0f);
    }

    bool scatter(const HitRecord& record, Vec3& attenuation, Ray& scattered) const override {
        float rel_ri = record.front_face ? ri : 1.0f / ri;
        float cos_theta = std::min(-record.dir.dot(record.normal), 1.0f);
        float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
        bool cannot_refract = sin_theta > rel_ri;
        
        Vec3 direction;
        if (cannot_refract || reflectance(cos_theta, rel_ri) > random_float()) {
            direction = record.dir.reflect(record.normal);
        } else {
            direction = record.dir.refract(record.normal, rel_ri);
        }
        
        scattered = Ray(record.pos, direction);
        attenuation = color;
        return true;
    }
};


#endif