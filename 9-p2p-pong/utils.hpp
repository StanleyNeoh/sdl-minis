#ifndef UTILS_HPP
#define UTILS_HPP

#include <chrono>

inline std::time_t curr_time() {
    using Clock = std::chrono::system_clock;
    return Clock::to_time_t(Clock::now());
}

#endif