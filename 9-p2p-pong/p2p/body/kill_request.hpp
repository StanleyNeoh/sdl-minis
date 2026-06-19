#ifndef P2P_BODY_KILL_REQUEST_HPP
#define P2P_BODY_KILL_REQUEST_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "lib/common/platform_socket.hpp"

namespace P2P {
    struct KillRequestBody {};

    template <>
    struct TV_BodyType<KillRequestBody> {
        constexpr static Type value = KillRequestType;
    };

    template <>
    struct TV_IsWireable<KillRequestBody> {
        constexpr static bool value = false;
    };

    template <>
    struct TV_IsDisconnect<KillRequestBody> {
        constexpr static bool value = true;
    };
}

#endif