#ifndef PARTICLEBOX_TEXTURES
#define PARTICLEBOX_TEXTURES

#include <SDL.h>
#include <vector>

template <int W, int H, u_int32_t Color = 0xFFFFFFFF>
struct Circle {
    SDL_Texture* tex = NULL;

    Circle(SDL_Renderer* renderer) {
        if (tex != NULL) return;
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
        std::vector<Uint32> pixels(W * H);
        int pitch = W * sizeof(u_int32_t);
        float wr = static_cast<float>(W) / 2;
        float hr = static_cast<float>(H) / 2;
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                float dx = (static_cast<float>(x) - wr) / wr;
                float dy = (static_cast<float>(y) - hr) / hr;
                if (dx * dx + dy * dy <= 1.0f) {
                    pixels[y * W + x] = 0xFFFFFFFF;
                } else {
                    pixels[y * W + x] = 0x00000000;
                }
            }
        }
        SDL_UpdateTexture(tex, NULL, pixels.data(), pitch);
    }

    ~Circle() {
        if (tex != NULL) {
            SDL_DestroyTexture(tex);
            tex = NULL;
        }
    }
};


#endif