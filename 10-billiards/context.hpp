#ifndef SDL_CONTEXT_HPP
#define SDL_CONTEXT_HPP

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>

namespace Context {
    extern SDL_Window* window;
    extern SDL_Renderer* renderer;
    extern ImGuiIO* io;
    extern const ImVec4 clear_color;

    void init();
    void destroy();
}

#endif