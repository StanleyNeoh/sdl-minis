#include "stager.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "demo.hpp"
#include "../context.hpp"
#include <SDL.h>

namespace Apps {
namespace Stager {
    using Context::renderer;
    using Context::window;
    using Context::io;
    using Context::clear_color;

    bool done = false;
    int selectIndex = -1;
    void drawSide() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2{0, 0}, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(100, viewport->Size.y));
        ImGui::Begin("Select App", nullptr, ImGuiWindowFlags_NoCollapse);
        if (ImGui::Button("Demo")) {
            selectIndex = 0;
        };
        if (ImGui::Button("None")) {
            selectIndex = -1;
        }
        ImGui::End();
    }

    void drawMain() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2{100, 0}, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - 100, viewport->Size.y));
        ImGui::Begin("Stage", nullptr, ImGuiWindowFlags_NoCollapse);
        if (selectIndex == 0) {
            Apps::Demo::draw();
        } else {
            ImGui::Text("None selected");
        }
        ImGui::End();
    }

    void run() {
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

            drawSide();
            drawMain();

            // Rendering
            ImGui::Render();
            SDL_RenderSetScale(renderer, io->DisplayFramebufferScale.x, io->DisplayFramebufferScale.y);
            SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
            SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
            SDL_RenderPresent(renderer);
        }
    }
}
}