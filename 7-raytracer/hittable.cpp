#include "vec3.hpp"
#include "hittable.hpp"
#include "ray.hpp"

void HitRecord::set_face_normal(const Ray& ray, const Vec3& outward_normal) {
    front_face = ray.dir.dot(outward_normal) < 0;
    normal = front_face ? outward_normal : -outward_normal;
}

bool Hittables::hit(Ray& ray, Interval trange, HitRecord& record) const {
    bool has_hit = false;
    for (Hittable* ptr: hittables) {
        if (ptr->hit(ray, trange, record)) {
            trange.max_t = record.t;
            has_hit = true;
        }
    }
    if (!has_hit) {
        float a = 0.5 * (ray.dir.unit().y + 1.0);
        ray.color = (1.0 - a) * Vec3{1.0, 1.0, 1.0} + a * Vec3{0.5, 0.7, 1.0};
    }
    return !has_hit;
}

bool Sphere::hit(Ray& ray, Interval trange, HitRecord& record) const {
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

    record.t = root;
    record.p = ray.at(root);
    record.set_face_normal(ray, (record.p - center) / rad);
    ray.color = 0.5 * (record.normal + Vec3{1,1,1});
    return true;
}

bool Cube::hit(Ray& ray, Interval trange, HitRecord& record) const {
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
    for (int i = 0; i < 2; i++) {
        if (!trange.contains(hits[i].t)) continue;
        record.t = hits[i].t;
        record.p = ray.at(record.t);
        record.set_face_normal(ray, hits[i].normal);
        ray.color = 0.5 * (record.normal + Vec3{1,1,1});
        return true;
    }
    return false;
}

bool Plane::hit(Ray& ray, Interval trange, HitRecord& record) const {
    int count = 0;
    Ray::CutPlaneNormalRes res;
    if (!ray.cutPlaneNormal(pos, normal, res)) return false;
    if (!trange.contains(res.t)) return false;
    record.t = res.t;
    record.p = ray.at(record.t);
    record.set_face_normal(ray, normal);
    ray.color = Vec3{0, 0.5, 0};
    return true;
}
