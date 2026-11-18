#ifndef UTILS_PIPE_HPP
#define UTILS_PIPE_HPP

#include <stddef.h>
#include <atomic>

namespace Utils {

template <typename T, size_t N>
struct Pipe {
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

    void clear() {
        si.store(0, std::memory_order_relaxed);
        ei.store(0, std::memory_order_relaxed);
    }
};

}

#endif