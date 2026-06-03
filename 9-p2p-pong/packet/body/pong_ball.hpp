#ifndef PACKET_BODY_PONG_BALL_HPP
#define PACKET_BODY_PONG_BALL_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
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

        static constexpr size_t size() {
            return sizeof(PongBallBody) / sizeof(float) * sizeof(uint32_t);
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
}

#endif