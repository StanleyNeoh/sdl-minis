#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <shared_mutex>
#include <atomic>
#include <unordered_map>

enum GameState {
    GameState_Uninitialised,
    GameState_Available,
    GameState_InGame,
};

std::atomic<bool> is_running = true;
std::atomic<GameState> curr_state;

#endif