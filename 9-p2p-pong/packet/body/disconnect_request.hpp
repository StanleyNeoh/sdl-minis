#ifndef PACKET_BODY_DISCONNECT_REQUEST_HPP
#define PACKET_BODY_DISCONNECT_REQUEST_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
    struct DisconnectRequestBody {};

    template <>
    struct TV_BodyType<DisconnectRequestBody> {
        constexpr static Type value = DisconnectRequestType;
    };

    template <>
    struct TV_IsWireable<DisconnectRequestBody> {
        constexpr static bool value = false;
    };

    template <>
    struct TV_IsDisconnect<DisconnectRequestBody> {
        constexpr static bool value = true;
    };
}

#endif