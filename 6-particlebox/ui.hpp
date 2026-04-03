#ifndef PARTICLEBOX_UI
#define PARTICLEBOX_UI

#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <exception>

namespace UI {
    inline int win_w = 1280;
    inline int win_h = 800;
    inline float main_scale = 1.0;
    inline SDL_Window* window = nullptr;
    inline SDL_Renderer* renderer = nullptr;

    void init_sdl() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        {
            printf("Error: %s\n", SDL_GetError());
            std::terminate();
        }
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        window = SDL_CreateWindow("Particle Sim", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h, window_flags);
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

    void destroy() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    struct UI {
        UI() {
            init_sdl();
        }

        ~UI() {
            destroy();
        }
    };
};

#endif