#ifndef P2P_BODY_CONNECT_REQUEST_HPP
#define P2P_BODY_CONNECT_REQUEST_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "lib/common/platform_socket.hpp"

namespace P2P {
    struct ConnectRequestBody {
        sockaddr_in addr;
    };

    template <>
    struct TV_BodyType<ConnectRequestBody> {
        constexpr static Type value = ConnectRequestType;
    };

    template <>
    struct TV_IsWireable<ConnectRequestBody> {
        constexpr static bool value = false;
    };
}

#endif