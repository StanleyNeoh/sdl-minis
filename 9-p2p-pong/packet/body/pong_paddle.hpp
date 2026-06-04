#ifndef PACKET_BODY_PONG_PADDLE_HPP
#define PACKET_BODY_PONG_PADDLE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
    struct PongPaddleBody {
        float pos_x;
        float vel_x;

        bool serialize(char* buf) const {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float x) {
                uint32_t y = encode_f32(x);
                memcpy(ptr, &y, sizeof(y));
                ptr += sizeof(x);
            };
            mmemcpy(ptr, pos_x);
            mmemcpy(ptr, vel_x);
            return true;
        }

        bool deserialize(const char* buf) {
            const char* ptr = buf;
            auto mmemcpy = [](const char*& ptr, float& x) {
                uint32_t y;
                memcpy(&y, ptr, sizeof(x));
                x = decode_f32(y);
                ptr += sizeof(x);
            };
            mmemcpy(ptr, pos_x);
            mmemcpy(ptr, vel_x);
            return true;
        }
    };

    template <>
    struct TV_BodyType<PongPaddleBody> {
        constexpr static Type value = PongPaddleType;
    };

    template <>
    struct TV_IsWireable<PongPaddleBody> {
        constexpr static bool value = true;
    };

    template <>
    struct TV_BodySize<PongPaddleBody> {
        constexpr static size_t value = sizeof(PongPaddleBody) / sizeof(float) * sizeof(uint32_t);
    };

    template <>
    struct TO_Serialize<PongPaddleBody> {
        static bool f(const PongPaddleBody* body, char* buf) {
            return body->serialize(buf);
        }
    };

    template <>
    struct TO_Deserialize<PongPaddleBody> {
        static bool f(PongPaddleBody* body, const char* buf) {
            return body->deserialize(buf);
        }
    };
}

#endif