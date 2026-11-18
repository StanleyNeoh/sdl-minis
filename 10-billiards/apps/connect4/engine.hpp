#ifndef APPS_CONNECT4_ENGINE_HPP
#define APPS_CONNECT4_ENGINE_HPP

#include "board.hpp"
#include "../../utils/pipe.hpp"
#include <iostream>
#include <thread>

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
    Score scores[N];
};

enum EngineDataType {
    TypeNone,
    TypeMove,
    TypeSearch,
    TypeKill,
    TypeWin,
};
struct EngineData {
    EngineDataType type = TypeNone;
    union Data {
        char none = 0;
        struct {
            int col;
            Color turn;
            struct {
                int r = -1;
            } res;
        } move;
        struct {
            Color turn;
            int depth;
            bool play;
            struct {
                Action action;
            } res;
        } search;
        struct {
            struct {
                bool done = false;
            } res;
        } kill;

        struct {
            struct {
                Color color;
                BoardLoc marked[4];
            } res;
        } win;
    } data;
};
struct Engine {
    static constexpr int bufsize = 64;
    using ReqPipe = Utils::Pipe<EngineData, bufsize>;
    using ResPipe = Utils::Pipe<EngineData, bufsize>;

    Board board;
    mutable ReqPipe reqPipe;
    mutable ResPipe resPipe;
    std::thread _thread;

    static Action searchMove(Board& board, Color turn, int depth = 7);
    static void threadLoop(Board* board, ResPipe* resPipe, ReqPipe* reqPipe);
    void startThread();
    void stopThread();
    void clear();
};

}}

#endif