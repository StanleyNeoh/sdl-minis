#ifndef LIB_TEXTURES_BOARDFRAME_HPP
#define LIB_TEXTURES_BOARDFRAME_HPP

#include <SDL.h>

namespace Textures {

namespace BoardFrame {
    SDL_Texture* tex = NULL;

    constexpr int M = 6;
    constexpr int N = 7;

    constexpr int W = 1000;
    constexpr int H = 1000;

    constexpr int br = 20;
    constexpr int bw = W - 2 * br;
    constexpr int bh = H - 2 * br;

    constexpr int cbr = 2;
    constexpr int cw = static_cast<float>(bw) / N;
    constexpr int cwr = cw / 2.0;
    constexpr int ch = static_cast<float>(bh) / M;
    constexpr int chr = ch / 2.0;

    constexpr u_int32_t BG_COLOR = 0xFF000055;


    bool init(SDL_Renderer* renderer) {
        if (tex != NULL) return false;

        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        u_int32_t pix[H][W] = {0};
        for (int r = 0; r < br; r++) {
            int dy = br - r;
            int dx = br - SDL_sqrt(br * br - dy * dy);
            for (int c = dx; c < W - dx; c++) {
                pix[r][c] = pix[H-1-r][c] = BG_COLOR;
            }
        }

        for (int r = br; r < H - br; r++) {
            for (int c = 0; c < br; c++) {
                pix[r][c] = BG_COLOR;
            }
            for (int c = W - 1 - br; c < W; c++) {
                pix[r][c] = BG_COLOR;
            }
        }

        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                float tlx = cw * c + br;
                float tly = ch * r + br;
                float cx = tlx + cwr;
                float cy = tly + chr;
                for (int y = tly; y < tly + ch; y++) {
                    for (int x = tlx; x < tlx + cw; x++) {
                        float dx = (x - cx) / (cwr - 2 * cbr);
                        float dy = (y - cy) / (chr - 2 * cbr);
                        float d2 = dx * dx + dy * dy;
                        if (d2 > 1.0) {
                            pix[y][x] = BG_COLOR;
                        } 
                    }
                }
            }
        }

        SDL_UpdateTexture(tex, NULL, &pix, W * sizeof(u_int32_t));
        return true;
    }

    SDL_Point to_screen_point(const SDL_Rect& rect, const SDL_Point& tex_point) {
        SDL_Point screen_point;
        screen_point.x = static_cast<float>(tex_point.x * rect.w) / W + rect.x;
        screen_point.y = static_cast<float>(tex_point.y * rect.h) / H + rect.y;
        return screen_point;
    }

    SDL_Rect to_screen_rect(const SDL_Rect& rect, const SDL_Rect& tex_rect) {
        SDL_Rect screen_rect;
        screen_rect.x = static_cast<float>(tex_rect.x * rect.w) / W + rect.x;
        screen_rect.y = static_cast<float>(tex_rect.y * rect.h) / H + rect.y;
        screen_rect.w = static_cast<float>(tex_rect.w * rect.w) / W;
        screen_rect.h = static_cast<float>(tex_rect.h * rect.h) / H;
        return screen_rect;
    }
}
};


#endif