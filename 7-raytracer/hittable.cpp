#include "vec3.hpp"
#include "hittable.hpp"
#include "ray.hpp"

void HitRecord::set_face_normal(const Ray& ray, const Vec3& outward_normal) {
    front_face = ray.dir.dot(outward_normal) < 0;
    normal = front_face ? outward_normal : -outward_normal;
}

bool Sphere::hit(const Ray& ray, float ray_tmin, float ray_tmax, HitRecord& record) const {
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
    if (root <= ray_tmin || ray_tmax <= root) {
        root = (h + sqrtd) / a;
        if (root <= ray_tmin || ray_tmax <= root) {
            return false;
        }
    }

    record.t = root;
    record.p = ray.at(root);
    record.set_face_normal(ray, (record.p - center) / rad);
    return true;
}

void Sphere::color(Ray& ray) const {
    ray.color = Vec3{1, 0, 0};
}