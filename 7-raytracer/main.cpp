#include <exception>
#include <SDL.h>
#include "camera.hpp"
#include "hittable.hpp"

constexpr int win_w = 1280;
constexpr int win_h = 800;
SDL_Window* window;
SDL_Renderer* renderer;

struct UI {
    UI() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_VIDEO) != 0) {
            printf("Error: %s\n", SDL_GetError());
            std::terminate();
        }

        SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_INPUT_GRABBED);
        window = SDL_CreateWindow("Raytracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h, window_flags);
        if (window == nullptr)
        {
            printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            std::terminate();
        }
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        if (renderer == nullptr)
        {
            SDL_Log("Error creating SDL_Renderer!");
            std::terminate();
        }
    }

    ~UI() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main() {
    UI ui;

    SDL_Texture* screen_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, win_w, win_h);
    Camera<win_w, win_h> camera;
    UniformGlow greenGlowMat(Vec3{0, 1, 0});
    GradientGlow skyMat(Vec3{0, -1, 0}, Vec3{1, 1, 1}, Vec3{0.5, 0.7, 1.0});
    Lambertian floorMat(Vec3{0, 0.5, 0.01});
    Lambertian lambertMat(Vec3{0.5, 0.7, 0.7});
    Lambertian lambertMat2(Vec3{1.0, 0.5, 0.5});
    Metal metalMat(Vec3{0.8, 0.8, 0.9}, 0.01);
    Dielectric dielectricMat({1, 1, 1}, 1.3);
    Dielectric invDielectricMat({1, 1, 1}, 1 / 1.3);
    Dielectric dielectricMat2({1, 1, 0.8}, 1.13);

    Sphere sphere(Vec3{0, 0, 10}, 1.0f, &metalMat);
    Sphere sphere2(Vec3{-2.0f, 0, 10}, 1.0f, &dielectricMat);
    Sphere sphere2_1(Vec3{-2.0f, 0, 10}, 0.9f, &invDielectricMat);
    Sphere sphere3(Vec3{-4.0f, 0, 10}, 1.0f, &lambertMat2);
    Cube cube(Vec3{-3.0, -1.0, 6}, Vec3{2, 2 ,2}, &dielectricMat2);
    Plane floor(Vec3{0, -1, 0}, Vec3{0, 1, 0}, &lambertMat);
    WireFrame wire(Vec3{-3.0, -1.0, 9}, Vec3{2, 2, 2}, &greenGlowMat);
    Hittables hittables(&skyMat);
    hittables.add(&sphere);
    hittables.add(&sphere2);
    hittables.add(&sphere2_1);
    hittables.add(&sphere3);
    hittables.add(&cube);
    hittables.add(&floor);
    hittables.add(&wire);

    bool done = false;
    Uint32 last_time = SDL_GetTicks();
    while (!done) {
        Uint32 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                done = true;
                break;
            default:
                camera.handle_event(event);
                break;
            }
        }

        camera.update(dt);
        camera.render(screen_tex, hittables);
        SDL_RenderCopy(renderer, screen_tex, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}