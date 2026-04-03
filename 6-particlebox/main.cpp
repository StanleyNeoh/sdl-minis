#include "ui.hpp"
#include "particle.hpp"

int main() {
    UI::UI ui;
    Entity::ParticleBox pb(1280, 800);
    pb.random_init(100);
    pb.init_tex(UI::renderer);
    for (auto& p: pb.particles) {
        p.init_tex(UI::renderer);
    }

    bool done = false;
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

        pb.render(UI::renderer, NULL);
        pb.step();

        SDL_RenderPresent(UI::renderer);
        SDL_Delay(10);
    }

    pb.destroy_tex();
    for (auto& p: pb.particles) {
        p.destroy_tex();
    }
}