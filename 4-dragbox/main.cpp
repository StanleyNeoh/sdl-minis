#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include "lib/element.hpp"
#include "lib/app.hpp"

#ifndef BLOB_DIR
#define BLOB_DIR "./"
#endif

int main(int argc, char* argv[]) {
    App app;
    app.init("Hello", 500, 500);

    TTF_Font* font = TTF_OpenFont(BLOB_DIR "ttf/google-sans/ProductSans-Regular.ttf", 28);
    Text text1("Hello World", font);
    Text text2("GLOBAL", font);
    DragBox drag_box;
    drag_box.add_child(static_cast<Element*>(&text1), 0.25, 0.5, 0.3, 0.3);
    drag_box.add_child(static_cast<Element*>(&text2), 0.75, 0.5, 0.3, 0.3);
    app.add_scene("drag_box", &drag_box);

    app.init_scenes();
    app.run("drag_box");

    return 0;

    // SDL_Window* window;
    // if (!init(&window, 500, 500)) return 1;
    // SDL_Surface* winSurface = SDL_GetWindowSurface(window);
    // SDL_FillRect(winSurface, NULL, SDL_MapRGB(winSurface->format, 255, 255, 255));

    // if (font == NULL) {
    //     std::cerr << "Failed to load font: " << TTF_GetError() << "\n";
    //     return 1;
    // }

    // SDL_Rect rect{0, 0, 200, 200};
    // SDL_BlitSurface(text, NULL, winSurface, &rect);
    // SDL_UpdateWindowSurface(window);

    // bool quit = false;
    // while (!quit) {
    //     SDL_Event e;
    //     while (SDL_PollEvent(&e)) {
    //         switch(e.type) {
    //         case SDL_QUIT:
    //             quit = true;
    //             break;
    //         default:
    //             break;
    //         }
    //     }
    //     SDL_Delay(100);
    // }
    // return 0;
}