#ifndef LIB_BUFFER_HPP
#define LIB_BUFFER_HPP

#include <iostream>
#include <array>
#include <atomic>
#include "utils.hpp"

template <typename T, size_t N>
struct CircularBuffer {
    static_assert(((N & (N - 1)) == 0), "N must be a power of 2");
    static constexpr size_t MASK = N - 1;

    std::array<T, N> buf;
    std::atomic<size_t> si = 0; // Shifted by single consumer only
    std::atomic<size_t> ei = 0; // Shifted by single producer only

    size_t size() {
        return ei.load(std::memory_order_acquire) - si.load();
    }

    bool empty() {
        return size() == 0;
    }

    bool full() {
        return size() == N;
    }
    
    bool pop(T& out) {
        if (empty()) return false;
        size_t i = si.load();
        out = buf[i & MASK]; // When empty(), buf[i & MASK] must have been updated, use acq rel
        si.store(i+1, std::memory_order_release);
        return true;
    }

    bool push(const T& in) {
        if (full()) return false;
        size_t i = ei.load();
        buf[i & MASK] = in;
        ei.store(i+1, std::memory_order_release); // ensures buf is written before consumer reads.
        return true;
    }

    void block_push(const T& in) {
        while (!push(in));
    }

};

enum class NetworkEventType {
    MOVE,
    GAME_END
};

struct Move {
    NetworkEventType type;
    Cell key;
    int r;
    int c;
};

struct GameEnd {
    NetworkEventType type;
    Cell winner;
    std::array<GridLoc, 4> marked;
};

union BotEvent {
    NetworkEventType type;
    Move move;
    GameEnd game_end;
};

using BotEventBuf = CircularBuffer<BotEvent, 8>;

union UiEvent {
    NetworkEventType type;
    Move move;
};

using UiEventBuf = CircularBuffer<UiEvent, 8>;

struct Network {
    UiEventBuf ui_events;
    BotEventBuf bot_events;
};

#endif