#ifndef PACKET_BODY_CONNECT_RESPONSE_HPP
#define PACKET_BODY_CONNECT_RESPONSE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
    struct ConnectResponseBody {
        sockaddr_in addr;
        bool is_master;
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