#ifndef PARTICLEBOX_UTILS
#define PARTICLEBOX_UTILS
#include <cstdint>

int get_rand_int(int l, int r);
float get_rand_float(float l, float r);
uint32_t get_rand_color();

struct RGB { uint8_t r, g, b; };
RGB hue_to_rgb(float hue);

#endif