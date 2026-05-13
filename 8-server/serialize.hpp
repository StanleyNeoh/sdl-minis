#ifndef SERIALIZE_HPP
#define SERIALIZE_HPP

#include "utils.hpp"
#include "network.hpp"

enum struct Ops: int {
    Uninitialized,
    ServerMessage,
    Join,
    Leave,
    ListRooms,
    ListRoomMembers,
    CreateRoom,
    RoomMessage,
};

struct ServerMessageBody {
    static constexpr Ops ops = Ops::ServerMessage;
    std::string msg;

    bool send(int socket) const {
        return send_i32(socket, msg.size())
            && send_exact(socket, msg.data(), msg.size());
    }

    bool recv(int socket) {
        int ssize = 0;
        return recv_i32(socket, ssize)
            && recv_string(socket, ssize, msg);
    }
};

struct JoinBody {
    static constexpr Ops ops = Ops::Join;
    int room_id;

    bool send(int socket) const {
        return send_i32(socket, room_id);
    }

    bool recv(int socket) {
        return recv_i32(socket, room_id);
    }
};

struct LeaveBody {
    static constexpr Ops ops = Ops::Leave;
    int room_id;

    bool send(int socket) const {
        return send_i32(socket, room_id);
    }

    bool recv(int socket) {
        return recv_i32(socket, room_id);
    }
};

struct ListRoomsBody {
    static constexpr Ops ops = Ops::ListRooms;
    bool send(int socket) const { return true; }
    bool recv(int socket) { return true; }
};

struct ListRoomMembersBody {
    static constexpr Ops ops = Ops::ListRoomMembers;
    int room_id;

    bool send(int socket) const { 
        return send_i32(socket, room_id);
    }

    bool recv(int socket) {
        return recv_i32(socket, room_id);
    }
};

struct CreateRoomBody {
    static constexpr Ops ops = Ops::CreateRoom;
    std::string room_name;

    bool send(int socket) const {
        return send_i32(socket, room_name.size()) 
            && send_exact(socket, room_name.data(), room_name.size());
    }

    bool recv(int socket) {
        int ssize = 0;
        return recv_i32(socket, ssize)
            && recv_string(socket, ssize, room_name);
    }
};

struct RoomMessageBody {
    static constexpr Ops ops = Ops::RoomMessage;
    int room_id;
    std::string msg;

    bool send(int socket) const {
        return send_i32(socket, room_id)
            && send_i32(socket, msg.size())
            && send_exact(socket, msg.data(), msg.size());
    }

    bool recv(int socket) {
        int ssize = 0;
        return recv_i32(socket, room_id)
            && recv_i32(socket, ssize)
            && recv_string(socket, ssize, msg);
    }
};

struct Body {
    Ops ops;
    union {
        ServerMessageBody server_msg;
        JoinBody join_body;
        LeaveBody leave_body;
        ListRoomsBody list_rooms_body;
        ListRoomMembersBody list_room_members_body;
        CreateRoomBody create_room_body;
        RoomMessageBody room_msg_body;
    };

    Body(): ops(Ops::Uninitialized) {}

    ~Body() { destroy(); }

    void destroy() {
        switch (ops) {
        case Ops::ServerMessage:
            server_msg.~ServerMessageBody();
            break;
        case Ops::Join:
            join_body.~JoinBody();
            break;
        case Ops::Leave:
            leave_body.~LeaveBody();
            break;
        case Ops::ListRooms:
            list_rooms_body.~ListRoomsBody();
            break;
        case Ops::ListRoomMembers:
            list_room_members_body.~ListRoomMembersBody();
            break;
        case Ops::CreateRoom:
            create_room_body.~CreateRoomBody();
            break;
        }
    }

    template <typename T>
    T& emplace() {
        destroy();
        ops = T::ops;
        return *new (&server_msg) T();
    }
};


template <typename T>
bool send_body(int socket, const T& body) {
    return send_i32(socket, static_cast<int>(T::ops))
        && body.send(socket);
}

bool recv_body(int socket, Body& body) {
    int opscode;
    if (!recv_i32(socket, opscode)) return false;
    std::cout << " RECV " << opscode << "\n";
    switch (opscode) {
    case static_cast<int>(Ops::ServerMessage):
        return body.emplace<ServerMessageBody>().recv(socket);
    case static_cast<int>(Ops::Join):
        return body.emplace<JoinBody>().recv(socket);
    case static_cast<int>(Ops::Leave):
        return body.emplace<LeaveBody>().recv(socket);
    case static_cast<int>(Ops::ListRooms):
        return body.emplace<ListRoomsBody>().recv(socket);
    case static_cast<int>(Ops::ListRoomMembers):
        return body.emplace<ListRoomMembersBody>().recv(socket);
    case static_cast<int>(Ops::CreateRoom):
        return body.emplace<CreateRoomBody>().recv(socket);
    case static_cast<int>(Ops::RoomMessage):
        return body.emplace<RoomMessageBody>().recv(socket);
    default:
        return false;
    }
}

#endif