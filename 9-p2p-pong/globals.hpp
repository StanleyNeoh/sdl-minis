#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <shared_mutex>
#include <atomic>
#include <unordered_map>

enum AppState {
    AppState_Uninitialised,
    AppState_Available,
    AppState_InGame,
    AppState_Closed,
};

std::atomic<AppState> currState = AppState_Uninitialised;

#endif