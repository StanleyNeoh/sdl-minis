#ifndef P2P_BODY_PONG_PADDLE_HPP
#define P2P_BODY_PONG_PADDLE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "common/platform_socket.hpp"

namespace P2P {
    struct PongPaddleBody {
        float pos_y;
        float vel_y;
        float p_acc_y;
        float p_vel_y;

        bool serialize(char* buf) const {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float x) {
                uint32_t y = encode_f32(x);
                memcpy(ptr, &y, sizeof(y));
                ptr += sizeof(x);
            };
            mmemcpy(ptr, pos_y);
            mmemcpy(ptr, vel_y);
            mmemcpy(ptr, p_acc_y);
            mmemcpy(ptr, p_vel_y);
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
            mmemcpy(ptr, pos_y);
            mmemcpy(ptr, vel_y);
            mmemcpy(ptr, p_acc_y);
            mmemcpy(ptr, p_vel_y);
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