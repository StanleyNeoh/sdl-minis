#ifndef P2P_BODY_PONG_CONFIG_HPP
#define P2P_BODY_PONG_CONFIG_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "lib/common/platform_socket.hpp"

namespace P2P {
    struct PongConfigBody {
        float width;
        float height;
        float pad_h;
        float pad_m;
        float ball_pos_x;
        float ball_pos_y;
        float ball_vel_x;
        float ball_vel_y;
        float ball_r;
        float left_pos_x;
        float left_pos_y;
        float left_w;
        float right_pos_x;
        float right_pos_y;
        float right_w;

        bool serialize(char* buf) const {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float x) {
                uint32_t y = encode_f32(x);
                memcpy(ptr, &y, sizeof(y));
                ptr += sizeof(x);
            };
            mmemcpy(ptr, width);
            mmemcpy(ptr, height);
            mmemcpy(ptr, pad_h);
            mmemcpy(ptr, pad_m);
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
            mmemcpy(ptr, ball_r);
            mmemcpy(ptr, left_pos_x);
            mmemcpy(ptr, left_pos_y);
            mmemcpy(ptr, left_w);
            mmemcpy(ptr, right_pos_x);
            mmemcpy(ptr, right_pos_y);
            mmemcpy(ptr, right_w);
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
            mmemcpy(ptr, width);
            mmemcpy(ptr, height);
            mmemcpy(ptr, pad_h);
            mmemcpy(ptr, pad_m);
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
            mmemcpy(ptr, ball_r);
            mmemcpy(ptr, left_pos_x);
            mmemcpy(ptr, left_pos_y);
            mmemcpy(ptr, left_w);
            mmemcpy(ptr, right_pos_x);
            mmemcpy(ptr, right_pos_y);
            mmemcpy(ptr, right_w);
            return true;
        }
    };

    template <>
    struct TV_BodyType<PongConfigBody> {
        constexpr static Type value = PongConfigType;
    };

    template <>
    struct TV_IsWireable<PongConfigBody> {
        constexpr static bool value = true;
    };

    template <>
    struct TV_BodySize<PongConfigBody> {
        constexpr static size_t value = sizeof(PongConfigBody) / sizeof(float) * sizeof(uint32_t);
    };

    template <>
    struct TO_Serialize<PongConfigBody> {
        static bool f(const PongConfigBody* body, char* buf) {
            return body->serialize(buf);
        }
    };

    template <>
    struct TO_Deserialize<PongConfigBody> {
        static bool f(PongConfigBody* body, const char* buf) {
            return body->deserialize(buf);
        }
    };
}

#endif