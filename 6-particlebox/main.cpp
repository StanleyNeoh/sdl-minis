#include "ui.hpp"
#include "textures.hpp"
#include "particle_box.hpp"

void NewFrame() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

int main() {
    UI::UI ui;
    Circle<100, 100> circle(UI::renderer);

    ParticleBox pb(UI::renderer, circle.tex, 1280, 800);

    bool done = false;
    bool paused = false;
    float rad = 1.0;
    int num_particles = 100;
    int steps_per_sec = 10;
    int padding = 20;
    SDL_Rect content_rect{padding, padding, UI::win_w-2*padding, UI::win_h-2*padding};
    Uint64 last_time = SDL_GetPerformanceCounter();
    double step_accumulator = 0.0;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    done = true;
                    break;
            }
        }
        SDL_SetRenderDrawColor(UI::renderer, 0, 0, 0, 0);
        SDL_RenderClear(UI::renderer);

        pb.rad = rad;
        pb.meet_target(num_particles);
        SDL_Texture* tex = pb.render(UI::renderer);
        SDL_RenderCopy(UI::renderer, tex, NULL, &content_rect);

        NewFrame();
        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Particles: %d", (int)pb.particles.size());
        ImGui::SameLine();
        if (ImGui::Button(paused ? "Resume" : "Pause")) {
            paused = !paused;
        }
        ImGui::SliderInt("Count", &num_particles, 1, 20000);
        ImGui::SliderInt("Steps/sec", &steps_per_sec, 1, 100);
        ImGui::SliderFloat("Radius", &rad, 0.1, 20.0);
        ImGui::End();
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), UI::renderer);

        SDL_RenderPresent(UI::renderer);

        Uint64 now = SDL_GetPerformanceCounter();
        double elapsed_sec = (double)(now - last_time) / SDL_GetPerformanceFrequency();
        last_time = now;
        if (!paused) {
            step_accumulator += elapsed_sec * steps_per_sec;
            while (step_accumulator >= 1.0) {
                pb.step();
                step_accumulator -= 1.0;
            }
        }
    }
}