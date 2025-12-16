#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <tuple>

#include "lib/utils.hpp"
#include "lib/app.hpp"
#include "lib/control.hpp"

int main(int argc, char* args[]) {
    // Argument handlinge
    if (argc < 3) {
        std::cerr << "Required args: <width> <height>\n";
        return 1;
    }
    int width = cast_to<int>(args[1]);
    int height = cast_to<int>(args[2]);

    SketchPad sketchpad;
    Button red(sketchpad, 255, 0, 0);
    Button green(sketchpad, 0, 255, 0);
    Button blue(sketchpad, 0, 0, 255);
    Button white(sketchpad, 255, 255, 255);
    Button black(sketchpad, 0, 0, 0);

    using Btn_Col = ViewPortCol<Button, Button, Button, Button, Button>;
    Btn_Col btnCol(
        Frame<Button>{red, 1}, 
        Frame<Button>{green, 1},
        Frame<Button>{blue, 1},
        Frame<Button>{white, 1},
        Frame<Button>{black, 1}
    );

    ViewPortRow<Btn_Col, SketchPad> row(
        Frame<Btn_Col>(btnCol, 1),
        Frame<SketchPad>(sketchpad, 10)
    );
    
    SDLApp app("SketchPad", width, height);
    app.run(row);
    return 0;
}