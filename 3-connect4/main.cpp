#include <iostream>
#include <SDL.h>

#include "lib/app.hpp"
#include "lib/engine.hpp"
#include "lib/entity.hpp"

template<typename T>
int engine_thread(void* arg) {
    std::cout << "Starting engine thread" << std::endl;
    T* eng = reinterpret_cast<T*>(arg);
    eng->bot_loop();
    return 0;
}

int main(int argc, char* argv[]) {
    App app("Main", 500, 500);
    Engine eng;
    Board<6, 7> board(eng);
    SDL_Thread* th = SDL_CreateThread(engine_thread<decltype(eng)>, "Engine Thread", reinterpret_cast<void*>(&eng));
    app.run(board);
    SDL_DetachThread(th);
    return 0;
}