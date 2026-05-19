#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <shared_mutex>
#include <atomic>
#include <unordered_map>

struct LocData;

enum GameState {
    GameState_Uninitialised,
    GameState_Available,
    GameState_InGame,
};

std::atomic<bool> is_running;
std::atomic<GameState> curr_state;
std::shared_mutex neighbour_ips_mut;
std::unordered_map<size_t, LocData> neighbour_ips;

#endif