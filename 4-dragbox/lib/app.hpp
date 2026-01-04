#ifndef LIB_APP_HPP
#define LIB_APP_HPP

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <unordered_map>

#include "element/element.hpp"

struct App {
    std::unordered_map<const char*, Element*> scenes;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    const char* curr_scene = NULL;
    int w = 0;
    int h = 0;

    // Shared State
    SDL_Point mouse_pos;
    bool click_down;

    SDL_Renderer* get_renderer() {
        return renderer;
    }

    void add_scene(const char* scene_name, Element* scene) {
        scenes[scene_name] = scene;
    }

    bool init(const char* windowName, int w, int h) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << '\n';
            return false;
        }
        if (TTF_Init() == -1) {
            std::cerr << "TTF could not initialize! TTF_Error: " << TTF_GetError() << '\n';
            return false;
        }

        this->w = w;
        this->h = h;
        window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            std::cerr << "Window cannot be created: Error: " << SDL_GetError() << '\n';
            return false;
        }
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == NULL) {
            std::cerr << "Renderer cannot be created: Error: " << SDL_GetError() << '\n';
            return false;
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        return true;
    }

    bool init_scenes() {
        for (auto& scene: scenes) {
            scene.second->init(*this, 0, 0, w, h);
        }
        return true;
    }

    void run(const char* scene_name) {
        curr_scene = scene_name;

        bool quit = false;
        while (!quit) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                    case SDL_QUIT:
                        quit = true;
                        break;
                    case SDL_MOUSEMOTION:
                        mouse_pos.x = e.motion.x;
                        mouse_pos.y = e.motion.y;
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        click_down = true;
                        break;
                    case SDL_MOUSEBUTTONUP:
                        click_down = false;
                        break;
                };
                scenes[curr_scene]->handle_event(e);
            }
            SDL_RenderClear(renderer);
            scenes[curr_scene]->step_all();
            scenes[curr_scene]->draw_all();
            SDL_RenderPresent(renderer);
            SDL_Delay(10);
        }
    }
};

#endif