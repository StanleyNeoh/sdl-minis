#ifndef UTILS_HPP
#define UTILS_HPP

#include <chrono>
#include <thread>

inline std::time_t curr_time() {
    using Clock = std::chrono::system_clock;
    return Clock::to_time_t(Clock::now());
}

struct Backoff {
    size_t spinCount = 0;

    void backoff() {
        if (++spinCount < 64) {
            std::this_thread::yield();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            spinCount = 0;
        }
    }

    void reset() {
        spinCount = 0;
    }
};

#endif