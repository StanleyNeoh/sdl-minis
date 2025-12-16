#include <iostream>
#include <SDL.h>

#include "lib/app.hpp"
#include "lib/connect4.hpp"

int main(int argc, char* argv[]) {
    App app("Main", 500, 500);
    Board<6, 7> board;
    app.run(board);
    return 0;
}