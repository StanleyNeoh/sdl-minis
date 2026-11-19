#ifndef APPS_CONNECT4_SERIALIZER_HPP
#define APPS_CONNECT4_SERIALIZER_HPP

#include <string>
#include <bitset>

#include "board.hpp"
#include "engine.hpp"

namespace Apps {
namespace _Connect4 {
    template <typename... Ts>
    struct Serializer {
        static constexpr int nbits = (0 + ... + Serializer<Ts>::nbits);
        static constexpr int nbytes = (nbits + 7) / 8;
        std::string bytes;

        // 1. Mark the helper as constexpr
        static consteval auto calculate_offsets() {
            // Enforce compile-time array initialization
            constexpr std::size_t N = sizeof...(Ts);
            if constexpr (N == 0) return std::array<int, 0>{};

            constexpr int sizes[] = { Serializer<Ts>::nbits... };
            std::array<int, N> offsets{};
            int current = 0;
            for (std::size_t i = 0; i < N; ++i) {
                offsets[i] = current;
                current += sizes[i];
            }
            return offsets;
        }

        template <typename... Us>
        void serialize(Us... offset, const Ts& obj...) {
            bytes = (
                std::string{} + ... + 
                Serializer<Ts>().serialize(offset, obj).bytes()
            );
        }
    };

    template <>
    struct Serializer<_Board> {
        static constexpr int nbits = 2 * M * N;
        static constexpr int nbytes = (nbits + 7) / 8;
        std::string bytes;

        template <int i, int j = 0>
        u_char _extract(const Board& board)  {
            if constexpr ((i >= M * N) || j >= 8) {
                return 0;
            } else {
                return ((u_char) board.board[i / N][i % N] << j) | extract<i+1, j+2>(board);
            }
        }

        template <int i = 0>
        void _serialize(const Board& board) {
            if constexpr (i >= M * N) {
                return;
            } else {
                bytes[i / 4] = _extract<i>(board.board);
                serialize<i + 4>(board);
            }
        }

        auto& serialize(const Board& board) {
            bytes.assign(nbytes, 0);
            _serialize<0>(board);
            return *this;
        }

        template <int i, int j = 0>
        void _deserialize(Board& board) {
            if constexpr ((i >= M * N)) {
                return;
            } else {
                board[i / N][i % N]  = (bytes[i / 4] >> j);
                _deserialize<i + 1, (j + 2) % 8>(board);
            }
        }

        auto& deserialize(Board& board) {
            _deserialize<0>(board);
            return *this;
        }

        std::string bytes() {
            return bytes;
        }
    };

    struct Serializer<Score> {
        static constexpr int nbits = sizeof(Score);
        static constexpr int nbytes = (nbits + 7) / 8;
        std::string bytes;
        
        void serialize(const Score& score) {
            bytes.assign(nbytes, 0);
            memcpy(bytes.data(), &score, sizeof(score));
        }

        void deserialize(Score& score) {
            memcpy(bytes.data(), &score, sizeof(score));
        }
    };

}}

#endif