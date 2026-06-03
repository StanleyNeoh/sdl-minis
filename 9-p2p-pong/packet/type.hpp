#ifndef PACKET_TYPE_HPP
#define PACKET_TYPE_HPP

#include <cstdint>

namespace Packet {
    enum Type: uint32_t {
        UninitializedType,
        ConnectRequestType,
        ConnectResponseType,
        DisconnectRequestType,
        DisconnectResponseType,
        KillRequestType,
        MessageType,
        PongReadyType,
        PongConfigType,
        PongPaddleType,
        PongBallType,
    };
}

#endif