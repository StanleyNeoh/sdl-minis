#ifndef STAGER_HPP
#define STAGER_HPP

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "apps/demo.hpp"
#include "context.hpp"
#include <SDL.h>
#include <tuple>
#include <iostream>

namespace {
    template <typename T, int currInd = 0>
    struct _Meta {
        static void reinit(T& apps, int prevSelect, int newSelect) {
            if constexpr (currInd >= std::tuple_size<T>::value) {
                // pass
            } else {
                auto& app = get<currInd>(apps);
                if (currInd == prevSelect) app.deinit();
                if (currInd == newSelect) app.init();
                _Meta<T, currInd + 1>::reinit(apps, prevSelect, newSelect);
            }
        }

        static void select_draw(T& apps, int& select) {
            if constexpr (currInd >= std::tuple_size<T>::value) {
                if (ImGui::Button("None")) {
                    select = -1;
                }
            } else {
                auto& app = get<currInd>(apps);
                if (ImGui::Button(std::decay_t<decltype(app)>::name)) {
                    select = currInd;
                };
                _Meta<T, currInd + 1>::select_draw(apps, select);
            }
        }

        static void app_draw(T& apps, int currSelect, bool drawn = false) {
            if constexpr (currInd >= std::tuple_size<T>::value) {
                if (!drawn) ImGui::Text("None selected");
            } else {
                if (currInd == currSelect) {
                    get<currInd>(apps).draw();
                }
                _Meta<T, currInd + 1>::app_draw(apps, currSelect, drawn || currInd == currSelect);
            }
        }
    };
}

struct Stager {
    bool done = false;
    int selectIndex = -1;
    std::tuple<Apps::Demo> apps;

    void run() {
        using Context::renderer;
        using Context::window;
        using Context::io;
        using Context::clear_color;
        while (!done) {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    done = true;
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    done = true;
            }
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
            {
                SDL_Delay(10);
                return;
            }

            // Start the Dear ImGui frame
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            ImGuiViewport* viewport = ImGui::GetMainViewport();

            // Draw aside
            constexpr int asideWidth = 100;
            ImGui::SetNextWindowPos(ImVec2{0, 0}, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(asideWidth, viewport->Size.y));
            ImGui::Begin("Select App", nullptr, ImGuiWindowFlags_NoCollapse);
            int nextSelectIndex = selectIndex;
            _Meta<decltype(apps)>::select_draw(apps, nextSelectIndex);
            ImGui::End();

            // Setup selection
            if (selectIndex != nextSelectIndex) {
                _Meta<decltype(apps)>::reinit(apps, selectIndex, nextSelectIndex);
                selectIndex = nextSelectIndex;
            }

            // Draw main app
            ImGui::SetNextWindowPos(ImVec2{asideWidth, 0}, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - asideWidth, viewport->Size.y));
            ImGui::Begin("Stage", nullptr, ImGuiWindowFlags_NoCollapse);
            _Meta<decltype(apps)>::app_draw(apps, selectIndex);
            ImGui::End();

            // Rendering
            ImGui::Render();
            SDL_RenderSetScale(renderer, io->DisplayFramebufferScale.x, io->DisplayFramebufferScale.y);
            SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
            SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
            SDL_RenderPresent(renderer);
        }
    }
};

#endif