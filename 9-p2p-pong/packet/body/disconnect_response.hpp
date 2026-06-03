#ifndef PACKET_BODY_DISCONNECT_RESPONSE_HPP
#define PACKET_BODY_DISCONNECT_RESPONSE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
    struct DisconnectResponseBody {
        sockaddr_in addr;
    };

    template <>
    struct TV_BodyType<DisconnectResponseBody> {
        constexpr static Type value = DisconnectResponseType;
    };

    template <>
    struct TV_IsWireable<DisconnectResponseBody> {
        constexpr static bool value = false;
    };
}

#endif