#ifndef P2P_BODY_CONNECT_RESPONSE_HPP
#define P2P_BODY_CONNECT_RESPONSE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "common/platform_socket.hpp"

enum RoleState {
    RoleState_Uninitialised,
    RoleState_Master,
    RoleState_Client,
    RoleState_SinglePlayer
};

std::ostream& operator<<(std::ostream& o, const RoleState& state) {
    switch (state) {
        case RoleState_Uninitialised:
            o << "RoleState_Uninitialised";
            break;
        case RoleState_Master:
            o << "RoleState_Master";
            break;
        case RoleState_Client:
            o << "RoleState_Client";
            break;
        case RoleState_SinglePlayer:
            o << "RoleState_SinglePlayer";
            break;
    }
    return o;
};
namespace P2P {
    struct ConnectResponseBody {
        sockaddr_in addr;
        RoleState role_state;
    };

    template <>
    struct TV_BodyType<ConnectResponseBody> {
        constexpr static Type value = ConnectResponseType;
    };

    template <>
    struct TV_IsWireable<ConnectResponseBody> {
        constexpr static bool value = false;
    };
}

#endif