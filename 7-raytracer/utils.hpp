#ifndef RAYTRACER_UTILS
#define RAYTRACER_UTILS
#include <SDL.h>

inline Uint32* offset(Uint32* ptr, int n_bytes) {
    return reinterpret_cast<Uint32*>(reinterpret_cast<Uint8*>(ptr) + n_bytes);
}

inline Uint32 to_argb(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
    return (a << 24) | (r << 16) | (g << 8) | b;
}

#endif
