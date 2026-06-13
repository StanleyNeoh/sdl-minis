#ifndef PACKET_BODY_PONG_PROJ_HPP
#define PACKET_BODY_PONG_PROJ_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Packet {
    struct PongProjBody {
        float proj_pos_x;
        float proj_pos_y;
        float proj_vel_x;
        float proj_vel_y;
        float proj_life;

        bool serialize(char* buf) const {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float x) {
                uint32_t y = encode_f32(x);
                memcpy(ptr, &y, sizeof(y));
                ptr += sizeof(x);
            };
            mmemcpy(ptr, proj_pos_x);
            mmemcpy(ptr, proj_pos_y);
            mmemcpy(ptr, proj_vel_x);
            mmemcpy(ptr, proj_vel_y);
            mmemcpy(ptr, proj_life);
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
            mmemcpy(ptr, proj_pos_x);
            mmemcpy(ptr, proj_pos_y);
            mmemcpy(ptr, proj_vel_x);
            mmemcpy(ptr, proj_vel_y);
            mmemcpy(ptr, proj_life);
            return true;
        }
    };

    template <>
    struct TV_BodyType<PongProjBody> {
        constexpr static Type value = PongProjType;
    };

    template <>
    struct TV_IsWireable<PongProjBody> {
        constexpr static bool value = true;
    };

    template <>
    struct TV_BodySize<PongProjBody> {
        constexpr static size_t value = sizeof(PongProjBody) / sizeof(float) * sizeof(uint32_t);
    };

    template <>
    struct TO_Serialize<PongProjBody> {
        static bool f(const PongProjBody* body, char* buf) {
            return body->serialize(buf);
        }
    };

    template <>
    struct TO_Deserialize<PongProjBody> {
        static bool f(PongProjBody* body, const char* buf) {
            return body->deserialize(buf);
        }
    };
}

#endif