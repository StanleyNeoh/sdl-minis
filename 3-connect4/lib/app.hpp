
#ifndef LIB_APP_HPP
#define LIB_APP_HPP

#include <SDL.h>
#include <iostream>


struct App {
    SDL_Window* window;
    SDL_Renderer* window_renderer;
    int w;
    int h;
    bool success;

    App(const char* title, int w, int h): w(w), h(h) {
        success = init(title);
    }


    ~App() {
        SDL_DestroyRenderer(window_renderer);
        SDL_DestroyWindow(window);
        window_renderer = NULL;
        window = NULL;
        SDL_Quit();
    }

    App(const App&) = delete;
    App(App&&) noexcept = delete;
    App& operator=(const App&) = delete;
    App& operator=(App&&) noexcept = delete;

    bool init(const char* windowName) {
        window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            std::cerr << "Window cannot be created: Error: " << SDL_GetError() << '\n';
            return false;
        }
        window_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (window_renderer == NULL) {
            std::cerr << "Renderer cannot be created: Error: " << SDL_GetError() << '\n';
            return false;
        }
        SDL_SetRenderDrawColor(window_renderer, 255, 255, 255, 255);
        SDL_RenderClear(window_renderer);
        SDL_RenderPresent(window_renderer);
        return true;
    }

    template <typename T>
    void run(T& entity) {
        entity.init(window_renderer, 0, 0, w, h);

        bool quit = false;
        SDL_Event e;
        while (!quit) {
            entity.handle_network_event();
            while (SDL_PollEvent(&e)) {
                entity.handle_event(e, quit);
            }
            SDL_RenderClear(window_renderer);
            entity.draw();
            SDL_RenderPresent(window_renderer);
            SDL_Delay(10);
        }
        std::cout << "Ending SDL Loop" << std::endl;
    }
 };

#endif