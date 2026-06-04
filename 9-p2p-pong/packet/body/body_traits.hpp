#ifndef PACKET_BODY_BODY_TRAITS_HPP
#define PACKET_BODY_BODY_TRAITS_HPP

#include "../type.hpp"
#include <stddef.h>
#include <cstdint>
#include <type_traits>

namespace Packet {
    template<typename _Body>
    struct TV_BodySize {
        static constexpr size_t value = 0;
    };

    template<typename _Body>
    struct TV_IsWireable {
        static constexpr bool value = false;
    };

    template<typename _Body>
    struct TV_BodyType {
        static constexpr Type value = UninitializedType;
    };

    template<typename _Body>
    struct TV_IsDisconnect {
        static constexpr bool value = false;
    };

    template<typename _Body>
    struct TO_Serialize {
        static constexpr bool f(const _Body*, char*) {
            return false;
        }
    };

    template<typename _Body>
    struct TO_Deserialize {
        static constexpr bool f(_Body*, const char*) {
            return false;
        }
    };
}

#endif