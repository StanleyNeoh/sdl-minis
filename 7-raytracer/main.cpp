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
    UniformGlow floorMat(Vec3{0, 0.75, 0.01});
    SurfaceNormalGlow objMat;
    GradientGlow skyMat(Vec3{0, -1, 0}, Vec3{1, 1, 1}, Vec3{0.5, 0.7, 1.0});
    Lambertian lambertMat(Vec3{0.5, 0.5, 0.5});
    Metal metalMat(Vec3{0.8, 0.8, 0.9}, 0.3);

    Sphere sphere(Vec3{0, 0, 10}, 1.0f, &metalMat);
    Cube cube(Vec3{5, 0, 10}, Vec3{1, 1 ,1}, &lambertMat);
    Plane floor(Vec3{0, -1, 0}, Vec3{0, 1, 0}, &lambertMat);
    Hittables hittables(&skyMat);
    hittables.add(&sphere);
    hittables.add(&cube);
    hittables.add(&floor);

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