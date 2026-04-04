#ifndef RAYTRACER_RAY
#define RAYTRACER_RAY

#include <vector>
#include "hittable.hpp"
#include "vec3.hpp"
#include <SDL.h>

struct Ray {
    Vec3 orig;
    Vec3 dir;
    Vec3 color = {0, 0, 0};

    Vec3 at(float t) const;

    void cast(const std::vector<Hittable*>& hittables);

    Uint32 argb_color();
};

#endif