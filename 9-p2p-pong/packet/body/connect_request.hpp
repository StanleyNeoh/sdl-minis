#ifndef PACKET_BODY_CONNECT_REQUEST_HPP
#define PACKET_BODY_CONNECT_REQUEST_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
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