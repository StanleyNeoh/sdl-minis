#ifndef LOCDATA_HPP
#define LOCDATA_HPP

#include <utility>
#include <string_view>
#include <cstring>
#include <chrono>
#include "globals.hpp"
#include "platform_socket.hpp"

using Clock = std::chrono::system_clock;

struct LocData {
    in_addr_t address = 0;
    in_port_t port = 0;
    char name[32] = {0};
    time_t timestamp = 0;
    GameState state = GameState_Uninitialised;

    LocData(int port, std::string_view _name, in_addr_t address): port(port), address(address) {
        _name = _name.substr(0, 31);
        memcpy(name, _name.data(), _name.size());
    }
    LocData(int port, std::string_view _name): LocData(port, _name, 0) {}
    LocData(): LocData(0, "", 0) {};

    void refresh(GameState _state = GameState_Uninitialised) {
        timestamp = Clock::to_time_t(Clock::now());
        state = _state;
    }

    bool operator==(const LocData& other) const {
        return address == other.address && port == other.port;
    }

    std::size_t id() const {
        return address << 16 | port;
    }
};

template <>
struct std::hash<LocData> {
    std::size_t operator()(const LocData& locData) const noexcept {
        return locData.id();
    }
};

#endif