#ifndef P2P_BODY_PONG_BALL_HPP
#define P2P_BODY_PONG_BALL_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "common/platform_socket.hpp"

namespace P2P {
    struct PongBallBody {
        float ball_pos_x;
        float ball_pos_y;
        float ball_vel_x;
        float ball_vel_y;

        bool serialize(char* buf) const {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float x) {
                uint32_t y = encode_f32(x);
                memcpy(ptr, &y, sizeof(y));
                ptr += sizeof(x);
            };
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
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
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
            return true;
        }
    };

    template <>
    struct TV_BodyType<PongBallBody> {
        constexpr static Type value = PongBallType;
    };

    template <>
    struct TV_IsWireable<PongBallBody> {
        constexpr static bool value = true;
    };

    template <>
    struct TV_BodySize<PongBallBody> {
        constexpr static size_t value = sizeof(PongBallBody) / sizeof(float) * sizeof(uint32_t);
    };

    template <>
    struct TO_Serialize<PongBallBody> {
        static bool f(const PongBallBody* body, char* buf) {
            return body->serialize(buf);
        }
    };

    template <>
    struct TO_Deserialize<PongBallBody> {
        static bool f(PongBallBody* body, const char* buf) {
            return body->deserialize(buf);
        }
    };
}

#endif