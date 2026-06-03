#ifndef PACKET_BODY_PONG_CONFIG_HPP
#define PACKET_BODY_PONG_CONFIG_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
    struct PongConfigBody {
        float width;
        float height;
        float pad_w;
        float pad_m;
        float ball_pos_x;
        float ball_pos_y;
        float ball_vel_x;
        float ball_vel_y;
        float ball_r;
        float top_pos_x;
        float top_pos_y;
        float top_w;
        float bot_pos_x;
        float bot_pos_y;
        float bot_w;

        bool serialize(char* buf) const {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float x) {
                uint32_t y = encode_f32(x);
                memcpy(ptr, &y, sizeof(y));
                ptr += sizeof(x);
            };
            mmemcpy(ptr, width);
            mmemcpy(ptr, height);
            mmemcpy(ptr, pad_w);
            mmemcpy(ptr, pad_m);
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
            mmemcpy(ptr, ball_r);
            mmemcpy(ptr, top_pos_x);
            mmemcpy(ptr, top_pos_y);
            mmemcpy(ptr, top_w);
            mmemcpy(ptr, bot_pos_x);
            mmemcpy(ptr, bot_pos_y);
            mmemcpy(ptr, bot_w);
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
            mmemcpy(ptr, pad_w);
            mmemcpy(ptr, pad_m);
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
            mmemcpy(ptr, ball_r);
            mmemcpy(ptr, top_pos_x);
            mmemcpy(ptr, top_pos_y);
            mmemcpy(ptr, top_w);
            mmemcpy(ptr, bot_pos_x);
            mmemcpy(ptr, bot_pos_y);
            mmemcpy(ptr, bot_w);
            return true;
        }

        static constexpr size_t size() {
            return sizeof(PongConfigBody) / sizeof(float) * sizeof(uint32_t);
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
}

#endif