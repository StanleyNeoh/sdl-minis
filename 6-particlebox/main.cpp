#include "ui.hpp"
#include "textures.hpp"
#include "particle.hpp"

int main() {
    UI::UI ui;
    Circle<100, 100> circle(UI::renderer);

    ParticleBox pb(UI::renderer, circle.tex, 1280, 800);
    pb.random_init(100);

    bool done = false;
    int padding = 20;
    SDL_Rect content_rect{padding, padding, UI::win_w-2*padding, UI::win_h-2*padding};
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type) {
                case SDL_QUIT:
                    done = true;
                    break;
            }
        }
        SDL_SetRenderDrawColor(UI::renderer, 0, 0, 0, 0);
        SDL_RenderClear(UI::renderer);

        SDL_Texture* tex = pb.render(UI::renderer);
        SDL_RenderCopy(UI::renderer, tex, NULL, &content_rect);
        SDL_RenderPresent(UI::renderer);
        pb.step();
        SDL_Delay(10);
    }
}