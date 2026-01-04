#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include "lib/element/element.hpp"
#include "lib/element/text.hpp"
#include "lib/element/dragbox.hpp"
#include "lib/element/circle.hpp"
#include "lib/app.hpp"

#ifndef BLOB_DIR
#define BLOB_DIR "./"
#endif

int main(int argc, char* argv[]) {
    App app;
    app.init("Hello", 500, 500);

    TTF_Font* font = TTF_OpenFont(BLOB_DIR "ttf/google-sans/ProductSans-Regular.ttf", 70);
    Text text1("Hello World", font);
    Text text2("Bye", font);
    Text text3("Bob", font);
    Text text4("Henry", font);
    Text text5("Sharon", font);
    Text text6("Lisa", font, 45.0);
    Circle cir(5, {0, 255, 255, 255}, {255, 255, 0, 255});

    DragBox drag_box;
    drag_box.add_child(static_cast<Element*>(&text1), 0.1, 0.1, 0.3, 0.1);
    drag_box.add_child(static_cast<Element*>(&text2), 0.4, 0.1, 0.3, 0.1);
    drag_box.add_child(static_cast<Element*>(&text3), 0.7, 0.1, 0.3, 0.1);
    drag_box.add_child(static_cast<Element*>(&text4), 0.1, 0.6, 0.3, 0.1);
    drag_box.add_child(static_cast<Element*>(&text5), 0.4, 0.6, 0.3, 0.1);
    drag_box.add_child(static_cast<Element*>(&text6), 0.7, 0.6, 0.3, 0.1);
    drag_box.add_child(static_cast<Element*>(&cir), 0.5, 0.5, 0.2, 0.2);
    app.add_scene("drag_box", &drag_box);

    app.init_scenes();
    app.run("drag_box");

    return 0;
}