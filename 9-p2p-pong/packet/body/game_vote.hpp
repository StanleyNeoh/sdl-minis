#ifndef PACKET_BODY_GAME_VOTE_HPP
#define PACKET_BODY_GAME_VOTE_HPP

#include "body_traits.hpp"
#include "../type.hpp"
#include "../../platform_socket.hpp"

namespace Game {
    enum Type: uint32_t {
        Uninitialized,
        Pong,
    };
}

namespace Packet {
    struct GameVoteBody {
        Game::Type type;

        bool serialize(char* buf) const {
            uint32_t x = static_cast<uint32_t>(type);
            memcpy(buf, &x, sizeof(x));
            return true;
        }

        bool deserialize(const char* buf) {
            uint32_t x;
            memcpy(&x, buf, sizeof(x));
            type = static_cast<Game::Type>(x);
            return true;
        }
    };

    template <>
    struct TV_BodyType<GameVoteBody> {
        constexpr static Type value = GameVoteType;
    };

    template <>
    struct TV_IsWireable<GameVoteBody> {
        constexpr static bool value = true;
    };

    template <>
    struct TV_BodySize<GameVoteBody> {
        constexpr static size_t value = sizeof(uint32_t);
    };

    template <>
    struct TO_Serialize<GameVoteBody> {
        static bool f(const GameVoteBody* body, char* buf) {
            return body->serialize(buf);
        }
    };

    template <>
    struct TO_Deserialize<GameVoteBody> {
        static bool f(GameVoteBody* body, const char* buf) {
            return body->deserialize(buf);
        }
    };
}

#endif