#include "ray.hpp"
#include "utils.hpp"

Vec3 Ray::at(float t) const {
    return orig + t * dir;
}

void Ray::cast(const std::vector<Hittable*>& hittables) {
    for (auto ptr: hittables) {
        Hittable::HitRecord record;
        if (!ptr->hit(*this, 0.1, 20.0, record)) continue;
        ptr->color(*this);
        return;
    }
    float a = 0.5 * (dir.y + 1.0);
    color = (1.0 - a) * Vec3{1.0, 1.0, 1.0} + a * Vec3{0.5, 0.7, 1.0};
}

Uint32 Ray::argb_color() {
    return to_argb(255.999 * color.x, 255.999 * color.y, 255.999 * color.z, 255);
}