#ifndef SERIALIZE_HPP
#define SERIALIZE_HPP

#include <vector>
#include "utils.hpp"
#include "network.hpp"

enum struct Ops: int {
    Uninitialized,
    ServerMessage,
    Join,
    JoinResp,
    Leave,
    LeaveResp,
    ListRooms,
    ListRoomsResp,
    ListRoomMembers,
    ListRoomMembersResp,
    CreateRoom,
    CreateRoomResp,
    RoomMessage,
    RoomMessageResp,
};

struct ServerMessageBody {
    static constexpr Ops ops = Ops::ServerMessage;
    std::string msg;

    bool send(int socket) const {
        return send_string(socket, msg);
    }

    bool recv(int socket) {
        return recv_string(socket, msg);

    }
};

struct JoinBody {
    static constexpr Ops ops = Ops::Join;
    std::string roomname;

    bool send(int socket) const {
        return send_string(socket, roomname);
    }

    bool recv(int socket) {
        return recv_string(socket, roomname);
    }
};

struct JoinResp {
    static constexpr Ops ops = Ops::JoinResp;
    std::string roomname;
    int success;

    bool send(int socket) const {
        return send_string(socket, roomname)
            && send_i32(socket, success);
    }

    bool recv(int socket) {
        return recv_string(socket, roomname)
            && recv_i32(socket, success);
    }
};

struct LeaveBody {
    static constexpr Ops ops = Ops::Leave;
    std::string roomname;

    bool send(int socket) const {
        return send_string(socket, roomname);
    }

    bool recv(int socket) {
        return recv_string(socket, roomname);
    }
};

struct LeaveResp {
    static constexpr Ops ops = Ops::LeaveResp;
    std::string roomname;
    int success;

    bool send(int socket) const {
        return send_string(socket, roomname)
            && send_i32(socket, success);
    }

    bool recv(int socket) {
        return recv_string(socket, roomname)
            && recv_i32(socket, success);
    }
};

struct ListRoomsBody {
    static constexpr Ops ops = Ops::ListRooms;
    bool send(int socket) const { return true; }
    bool recv(int socket) { return true; }
};

struct ListRoomsResp {
    static constexpr Ops ops = Ops::ListRoomsResp;

    struct Room {
        std::string name;
        int num_members;

        Room() = default;
        Room(std::string_view name, int num_members): name(name), num_members(num_members) {}

        bool send(int socket) const {
            return send_string(socket, name)
                && send_i32(socket, num_members);
        }

        bool recv(int socket) {
            return recv_string(socket, name)
                && recv_i32(socket, num_members);
        }

        friend std::ostream& operator<<(std::ostream& o, const Room& room) {
            o << room.name << "(" << room.num_members << ")";
            return o;
        }
    };

    std::vector<Room> rooms;

    bool send(int socket) const {
        if (!send_i32(socket, rooms.size())) return false;
        for (auto& room: rooms) {
            if (!room.send(socket)) return false;
        }
        return true;
    }

    bool recv(int socket) {
        int nitems = 0;
        if (!recv_i32(socket, nitems)) return false;
        for (int i = 0; i < nitems; i++) {
            Room room;
            if (!room.recv(socket)) return false;
            rooms.push_back(std::move(room));
        }
        return true;
    }
};

struct ListRoomMembersBody {
    static constexpr Ops ops = Ops::ListRoomMembers;
    std::string roomname;

    bool send(int socket) const { 
        return send_string(socket, roomname);
    }

    bool recv(int socket) {
        return recv_string(socket, roomname);
    }
};

struct User {
    std::string name;
    std::string address;
    int fd;

    User() = default;
    User(const std::string& address, int fd): name(address), address(address), fd(fd) {}

    bool send(int socket) const {
        return send_string(socket, name)
        && send_string(socket, address)
        && send_i32(socket, fd);
    }

    bool recv(int socket) {
        return recv_string(socket, name)
        && recv_string(socket, address)
        && recv_i32(socket, fd);
    }

    friend std::ostream& operator<<(std::ostream& o, const User& conn) {
        o << "(" << conn.name << "/" << conn.address << "/" << conn.fd << ")";
        return o;
    }
};

struct ListRoomMembersResp {
    static constexpr Ops ops = Ops::ListRoomMembersResp;
    std::vector<User> members;

    ListRoomMembersResp() = default;

    bool send(int socket) const {
        if (!send_i32(socket, members.size())) return false;
        for (auto& member: members) {
            if (!member.send(socket)) return false;
        }
        return true;
    }

    bool recv(int socket) {
        int nitems = 0;
        if (!recv_i32(socket, nitems)) return false; 
        for (int i = 0; i < nitems; i++) {
            User member;
            if (!member.recv(socket)) return false;
            members.push_back(std::move(member));
        }
        return true;
    }
};

struct CreateRoomBody {
    static constexpr Ops ops = Ops::CreateRoom;
    std::string room_name;

    bool send(int socket) const {
        return send_string(socket, room_name);
    }

    bool recv(int socket) {
        return recv_string(socket, room_name);
    }
};

struct CreateRoomResp {
    static constexpr Ops ops = Ops::CreateRoomResp;
    std::string room_name;
    int success;

    bool send(int socket) const {
        return send_string(socket, room_name)
            && send_i32(socket, success);
    }

    bool recv(int socket) {
        return recv_string(socket, room_name)
            && recv_i32(socket, success);
    }
};

struct RoomMessageBody {
    static constexpr Ops ops = Ops::RoomMessage;
    std::string room_name;
    std::string msg;

    bool send(int socket) const {
        return send_string(socket, room_name)
            && send_string(socket, msg);
    }

    bool recv(int socket) {
        return recv_string(socket, room_name)
            && recv_string(socket, msg);
    }
};

struct RoomMessageResp {
    static constexpr Ops ops = Ops::RoomMessageResp;
    User user;
    std::string msg;

    RoomMessageResp() = default;
    RoomMessageResp(const User& user, std::string_view msg): user(user), msg(msg) {}

    bool send(int socket) const {
        return user.send(socket)
            && send_string(socket, msg);
    }

    bool recv(int socket) {
        int ssize = 0;
        return user.recv(socket)
            && recv_string(socket, msg);
    }
};

template <typename T>
bool send_body(int socket, const T& body) {
    return send_i32(socket, static_cast<int>(T::ops))
        && body.send(socket);
}

#endif