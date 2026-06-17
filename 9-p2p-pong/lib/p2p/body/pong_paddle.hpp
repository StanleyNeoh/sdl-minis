#ifndef P2P_BODY_PONG_PADDLE_HPP
#define P2P_BODY_PONG_PADDLE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "common/platform_socket.hpp"

namespace P2P {
    struct PongPaddleBody {
        constexpr static int N_PROJ = 1;
        struct ProjBody {
            float proj_pos_x;
            float proj_pos_y;
            float proj_vel_x;
            float proj_vel_y;
            float proj_life;
        };
        float pos_y;
        float vel_y;
        float p_acc_y;
        float p_vel_y;
        uint32_t proj_i;
        ProjBody projs[N_PROJ];

        static void mmemcpy(char*& ptr, float x) {
            uint32_t y = encode_f32(x);
            memcpy(ptr, &y, sizeof(y));
            ptr += sizeof(x);
        }
        static void mmemcpy(char*& ptr, uint32_t x) {
            uint32_t y = htonl(x);
            memcpy(ptr, &y, sizeof(y));
            ptr += sizeof(y);
        }

        static void mmempst(const char*& ptr, float& x) {
            uint32_t y;
            memcpy(&y, ptr, sizeof(x));
            x = decode_f32(y);
            ptr += sizeof(x);
        }
        static auto mmempst(const char*& ptr, uint32_t& x) {
            uint32_t y;
            memcpy(&y, ptr, sizeof(y));
            x = ntohl(x);
            ptr += sizeof(x);
        }

        bool serialize(char* buf) const {
            char* ptr = buf;
            mmemcpy(ptr, pos_y);
            mmemcpy(ptr, vel_y);
            mmemcpy(ptr, p_acc_y);
            mmemcpy(ptr, p_vel_y);
            mmemcpy(ptr, proj_i);
            for (int i = 0; i < 5; i++) {
                mmemcpy(ptr, projs[i].proj_pos_x);
                mmemcpy(ptr, projs[i].proj_pos_y);
                mmemcpy(ptr, projs[i].proj_vel_x);
                mmemcpy(ptr, projs[i].proj_vel_y);
                mmemcpy(ptr, projs[i].proj_life);
            }
            return true;
        }

        bool deserialize(const char* buf) {
            const char* ptr = buf;
            mmempst(ptr, pos_y);
            mmempst(ptr, vel_y);
            mmempst(ptr, p_acc_y);
            mmempst(ptr, p_vel_y);
            mmempst(ptr, proj_i);
            for (int i = 0; i < 5; i++) {
                mmempst(ptr, projs[i].proj_pos_x);
                mmempst(ptr, projs[i].proj_pos_y);
                mmempst(ptr, projs[i].proj_vel_x);
                mmempst(ptr, projs[i].proj_vel_y);
                mmempst(ptr, projs[i].proj_life);
            }
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