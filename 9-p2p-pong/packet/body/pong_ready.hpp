#ifndef PACKET_BODY_PONG_READY_HPP
#define PACKET_BODY_PONG_READY_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
    struct PongReadyBody {
        bool ready;

        bool serialize(char* buf) const {
            memcpy(buf, &ready, sizeof(ready));
            return true;
        }

        bool deserialize(const char* buf) {
            memcpy(&ready, buf, sizeof(ready));
            return true;
        }
    };

    template <>
    struct TV_BodyType<PongReadyBody> {
        constexpr static Type value = PongReadyType;
    };

    template <>
    struct TV_IsWireable<PongReadyBody> {
        constexpr static bool value = true;
    };

    template <>
    struct TV_BodySize<PongReadyBody> {
        constexpr static size_t value = sizeof(PongReadyBody::ready);
    };

    template <>
    struct TO_Serialize<PongReadyBody> {
        static constexpr bool f(const _Body* body, char* buf) {
            return body->serialize(buf);
        }
    };

    template <>
    struct TO_Deserialize<PongReadyBody> {
        static constexpr bool f(_Body* body, const char* buf) {
            return body->deserialize(buf);
        }
    }
}

#endif