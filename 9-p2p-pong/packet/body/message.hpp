#ifndef PACKET_BODY_MESSAGE_HPP
#define PACKET_BODY_MESSAGE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
    struct MessageBody {
        char message[128] = {0};

        bool serialize(char* buf) const {
            memcpy(buf, message, sizeof(message));
            return true;
        }

        bool deserialize(const char* buf) {
            memcpy(message, buf, sizeof(message));
            return true;
        }

        static constexpr size_t size() {
            return sizeof(message);
        }
    };

    template <>
    struct TV_BodyType<MessageBody> {
        constexpr static Type value = MessageType;
    };

    template <>
    struct TV_IsWireable<MessageBody> {
        constexpr static bool value = true;
    };

    template <>
    struct TV_BodySize<MessageBody> {
        constexpr static size_t value = sizeof(MessageBody::message);
    };
}

#endif