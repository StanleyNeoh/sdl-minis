#ifndef P2P_TYPE_HPP
#define P2P_TYPE_HPP

#include <cstdint>

namespace P2P {
    enum Type: uint32_t {
        UninitializedType,
        ConnectRequestType,
        ConnectResponseType,
        DisconnectRequestType,
        DisconnectResponseType,
        KillRequestType,
        MessageType,
        GameVoteType,
        PongConfigType,
        PongPaddleType,
        PongProjType,
        PongBallType,
    };
}

#endif