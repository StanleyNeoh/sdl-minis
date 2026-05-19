#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <shared_mutex>
#include <atomic>
#include <unordered_map>

enum GameState {
    GameState_Uninitialised,
    GameState_Available,
    GameState_InGame,
    GameState_Closed,
};

std::atomic<bool> isRunning = true;
std::atomic<GameState> currState = GameState_Uninitialised;

#endif