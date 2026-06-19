#ifndef P2P_TYPE_HPP
#define P2P_TYPE_HPP

#include <cstdint>
#include <sstream>
#include <string>

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

    enum RoleType {
        RoleType_Uninitialised,
        RoleType_Master,
        RoleType_Client,
        RoleType_SinglePlayer
    };

    inline std::ostream& operator<<(std::ostream& o, const RoleType& state) {
        switch (state) {
            case RoleType_Uninitialised:
                o << "RoleType_Uninitialised";
                break;
            case RoleType_Master:
                o << "RoleType_Master";
                break;
            case RoleType_Client:
                o << "RoleType_Client";
                break;
            case RoleType_SinglePlayer:
                o << "RoleType_SinglePlayer";
                break;
        }
        return o;
    };

    inline std::string to_string(RoleType role_type) {
        std::stringstream ss;
        ss << role_type;
        return ss.str();
    }


    enum GameType: uint32_t {
        GameType_Uninitialized,
        GameType_Pong,
        GameType_Shooter
    };
}

#endif