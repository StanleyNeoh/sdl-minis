#ifndef APPS_CONNECT4_ENGINE_HPP
#define APPS_CONNECT4_ENGINE_HPP

#include "board.hpp"
#include <iostream>
#define BIG 1000000

namespace Apps {
namespace _Connect4 {

struct Score {
    int a;
    int b;
    int c;

    bool operator==(const Score& other) const {
        return a == other.a && b == other.b && c == other.c;
    }

    bool operator<(const Score& other) const {
        if (a != other.a) return a < other.a;
        if (b != other.b) return b < other.b;
        return c < other.c;
    }

    bool operator>(const Score& other) const {
        return other < *this;
    }

    bool operator<=(const Score& other) const {
        return *this == other || *this < other;
    }

    bool operator>=(const Score& other) const {
        return other <= *this;
    }

    Score operator-() const {
        return Score{-a, -b, -c};
    }

    friend std::ostream& operator<<(std::ostream& o, const Score& score) {
        o << score.a << "," << score.b << "," << score.c;
        return o;
    }
};

struct Action {
    int bestMove;
    std::array<Score, 7> scores;
};

Action search_move(Board board, Color turn, int depth = 7);

}}

#endif