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

    ParticleBox<QuadTree> pb(UI::renderer, circle.tex, 1280, 800);
    ParticleBox<QuadTreeArena> pb_arena(UI::renderer, circle.tex, 1280, 800);

    bool done = false;
    bool paused = false;
    bool use_arena = true;
    int mt_mode = 1;
    float min_rad = 1.0f;
    float max_rad = 5.0f;
    float min_mass = 1.0f;
    float max_mass = 5.0f;
    float gravity = 0.0f;
    float restitution = 1.0f;
    int num_particles = 100;
    int min_frame_time_ms = 16;
    int padding = 20;
    SDL_Rect content_rect{padding, padding, UI::win_w-2*padding, UI::win_h-2*padding};
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

        auto step_and_render = [&](auto& active_pb) {
            active_pb.min_rad = min_rad;
            active_pb.max_rad = max_rad;
            active_pb.min_mass = min_mass;
            active_pb.max_mass = max_mass;
            active_pb.meet_target(num_particles);
            SDL_Texture* tex = active_pb.render(UI::renderer);
            SDL_RenderCopy(UI::renderer, tex, NULL, &content_rect);

            NewFrame();
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Particles: %d", (int)active_pb.particles.size());
            ImGui::SameLine();
            if (ImGui::Button(paused ? "Resume" : "Pause")) {
                paused = !paused;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                active_pb.particles.clear();
            }
            ImGui::Checkbox("Use Arena", &use_arena);
            const char* mt_modes[] = {"None", "Graph Coloring", "Mutex Locks", "Unsafe (No Lock)", "Naive (N^2)"};
            ImGui::Combo("MT Mode", &mt_mode, mt_modes, IM_ARRAYSIZE(mt_modes));
            int max_particles = (static_cast<MTMode>(mt_mode) == MTMode::naive) ? NAIVE_MAX_PARTICLES : 50000;
            if (num_particles > max_particles) num_particles = max_particles;
            ImGui::SliderInt("Count", &num_particles, 1, max_particles);
            ImGui::SliderInt("Min frame time (ms)", &min_frame_time_ms, 1, 100);
            ImGui::SliderFloat("Min Radius", &min_rad, 0.1f, 20.0f);
            ImGui::SliderFloat("Max Radius", &max_rad, 0.1f, 20.0f);
            if (min_rad > max_rad) { if (ImGui::IsItemActive()) min_rad = max_rad; else max_rad = min_rad; }
            ImGui::SliderFloat("Min Mass", &min_mass, 0.1f, 100.0f);
            ImGui::SliderFloat("Max Mass", &max_mass, 0.1f, 100.0f);
            if (min_mass > max_mass) { if (ImGui::IsItemActive()) min_mass = max_mass; else max_mass = min_mass; }
            ImGui::SliderFloat("Gravity", &gravity, -1.0f, 1.0f);
            ImGui::SliderFloat("Restitution", &restitution, 0.0f, 2.0f);
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Node pool: %d / %d", active_pb.quadtree.pool_node_used(), active_pb.quadtree.pool_node_capacity());
            ImGui::Text("Index pool: %d / %d", active_pb.quadtree.pool_indices_used(), active_pb.quadtree.pool_indices_capacity());
            ImGui::End();
            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), UI::renderer);

            SDL_RenderPresent(UI::renderer);

            if (!paused) {
                Uint32 start = SDL_GetTicks();
                active_pb.step(static_cast<MTMode>(mt_mode), gravity, restitution);
                Uint32 elapsed = SDL_GetTicks() - start;
                if (static_cast<int>(elapsed) < min_frame_time_ms) {
                    SDL_Delay(min_frame_time_ms - elapsed);
                }
            }
        };

        if (use_arena) {
            step_and_render(pb_arena);
        } else {
            step_and_render(pb);
        }
    }

    return 0;
}