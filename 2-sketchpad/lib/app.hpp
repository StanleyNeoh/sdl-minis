#ifndef LIB_APP_HPP
#define LIB_APP_HPP

#include <SDL2/SDL.h>
#include "sketchpad.hpp"

struct SDLApp {
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    int screenWidth;
    int screenHeight;
    bool success = false;

    SDLApp(const char* title, int screenWidth = 640, int screenHeight = 480): 
        screenWidth(screenWidth), screenHeight(screenHeight) 
    {
        success = init(title);
    }

    ~SDLApp() {
        destroy();
    }

    bool init(const char* title) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initalize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        } else {
            window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
            if (window == NULL) return false;
            screenSurface = SDL_GetWindowSurface(window);
            return true;
        }
    }

    bool destroy() {
        SDL_FreeSurface(screenSurface);
        SDL_DestroyWindow(window);
        window = NULL;
        SDL_Quit();
        return true;
    }

    template <typename T>
    void run(T& viewport) {
        // Initialize layout
        viewport.update_layout(screenWidth, screenHeight);
        std::cout << "HI" << std::endl;

        bool quit = false;
        SDL_Event e;
        while (!quit) {
            bool handled_event = false;
            while (SDL_PollEvent(&e)) {
                handled_event = viewport.handle_event(screenSurface, e, quit);
            }
            if (handled_event) {
                viewport.draw_surface(screenSurface);
                SDL_UpdateWindowSurface(window);
            }
            SDL_Delay(10);
        }
    }
};

#endif