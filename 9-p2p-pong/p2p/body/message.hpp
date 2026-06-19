#ifndef P2P_BODY_MESSAGE_HPP
#define P2P_BODY_MESSAGE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "lib/common/platform_socket.hpp"

namespace P2P {
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

    template <>
    struct TO_Serialize<MessageBody> {
        static bool f(const MessageBody* body, char* buf) {
            return body->serialize(buf);
        }
    };

    template <>
    struct TO_Deserialize<MessageBody> {
        static bool f(MessageBody* body, const char* buf) {
            return body->deserialize(buf);
        }
    };
}

#endif