#ifndef PACKET_BODY_UNINITIALIZED_HPP
#define PACKET_BODY_UNINITIALIZED_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
    struct UninitializedBody {};

    template <>
    struct TV_BodyType<UninitializedBody> {
        constexpr static Type value = UninitializedType;
    };

    template <>
    struct TV_IsWireable<UninitializedBody> {
        constexpr static bool value = false;
    };

    template <>
    struct TV_IsDisconnect<UninitializedBody> {
        constexpr static bool value = false;
    };
}

#endif