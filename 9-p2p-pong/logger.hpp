#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string_view>

struct Logger {
    std::ostream& out;
    std::string_view prefix;

    Logger(std::string_view prefix, std::ostream& ostream = std::cout): out(ostream), prefix(prefix) {}

    template <typename... Args>
    void log(Args&&... args) {
        // out << "[" << prefix << "] ";
        // (out << ... << std::forward<Args>(args));
        // out << "\n";
    }
};

#endif