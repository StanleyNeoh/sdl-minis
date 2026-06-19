#ifndef P2P_BODY_CONNECT_RESPONSE_HPP
#define P2P_BODY_CONNECT_RESPONSE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "lib/common/platform_socket.hpp"

namespace P2P {
    struct ConnectResponseBody {
        sockaddr_in addr;
        RoleType role_type;
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