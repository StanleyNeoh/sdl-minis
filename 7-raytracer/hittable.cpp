#include "vec3.hpp"
#include "hittable.hpp"
#include "ray.hpp"

void Hittable::HitRecord::set_face_normal(const Ray& ray, const Vec3& outward_normal) {
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

bool Cube::hit(const Ray& ray, float ray_tmin, float ray_tmax, HitRecord& record) const {
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
    if (ray.cutPlaneSpan(pos, {dim.x, 0, 0}, {0, dim.y, 0}, res)) check(res);
    if (ray.cutPlaneSpan(pos, {0, 0, dim.z}, {dim.x, 0, 0}, res)) check(res);
    if (ray.cutPlaneSpan(pos, {0, dim.y, 0}, {0, 0, dim.z}, res)) check(res);
    if (ray.cutPlaneSpan(opp, {-dim.x, 0, 0}, {0, -dim.y, 0}, res)) check(res);
    if (ray.cutPlaneSpan(opp, {0, 0, -dim.z}, {-dim.x, 0, 0}, res)) check(res);
    if (ray.cutPlaneSpan(opp, {0, -dim.y, 0}, {0, 0, -dim.z}, res)) check(res);
    if (count != 2) return false;
    if (hits[0].t > hits[1].t) std::swap(hits[0], hits[1]);
    if (hits[0].t >= ray_tmin && hits[0].t <= ray_tmax) {
        record.t = hits[0].t;
        record.p = hits[0].pos;
        record.set_face_normal(ray, hits[0].normal);
        return true;
    }
    if (hits[1].t >= ray_tmin && hits[1].t <= ray_tmax) {
        record.t = hits[1].t;
        record.p = hits[1].pos;
        record.set_face_normal(ray, hits[1].normal);
        return true;
    }
    return false;
}

void Cube::color(Ray& ray) const {
    ray.color = Vec3{0, 1, 0};
}

bool Plane::hit(const Ray& ray, float ray_tmin, float ray_tmax, HitRecord& record) const {
    int count = 0;
    Ray::CutPlaneNormalRes res;
    if (!ray.cutPlaneNormal(pos, normal, res)) return false;
    if (res.t < ray_tmin || res.t > ray_tmax) return false;
    record.t = res.t;
    record.p = res.pos;
    record.set_face_normal(ray, normal);
    return true;
}

void Plane::color(Ray& ray) const {
    ray.color = Vec3{0, 0, 1};
}