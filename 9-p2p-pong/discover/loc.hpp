#ifndef DISCOVER_LOC_HPP
#define DISCOVER_LOC_HPP

#include <sstream>
#include "common/common.hpp"

namespace Discover {
    constexpr static size_t MAX_NAME_SIZE = 31;

    struct Loc {
        enum State {
            IsHost,
            Available,
            Unavailable,
            Closed
        };

        sockaddr_in address;
        State state = Available;
        char name[MAX_NAME_SIZE + 1] = {0};
        time_t timestamp = 0;

        Loc() = default;
        Loc(const sockaddr_in& address, std::string_view _name, State state = Available): address(address), state(state) {
            _name = _name.substr(0, MAX_NAME_SIZE);
            memcpy(name, _name.data(), _name.size());
        }

        bool operator==(const Loc& other) const {
            return address == other.address;
        }
    };

    inline std::ostream& operator<<(std::ostream& o, const Loc& loc) {
        o << loc.name << "=" << loc.address;
        return o;
    }

    inline std::string to_string(const Loc& loc) {
        std::stringstream ss;
        ss << loc;
        return ss.str();
    }

    inline std::string to_string(const Loc::State& state) {
        switch (state) {
            case Loc::IsHost:
                return "Is Host";
            case Loc::Available:
                return "Available";
            case Loc::Unavailable:
                return "Unavailable";
            case Loc::Closed:
                return "Closed";
            default:
                return "?";
        }
    }

    inline std::ostream& operator<<(std::ostream& o, const Loc::State& state) {
        o << to_string(state);
        return o;
    }
}

#endif