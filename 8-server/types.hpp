#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <shared_mutex>
#include <mutex>
#include <ostream>
#include <unordered_set>
#include "network.hpp"

struct Room {
    std::string roomname;
    mutable std::shared_mutex room_users_mut;
    std::unordered_set<int> room_users;

    Room(std::string& roomname): roomname(roomname) {}

    size_t get_num_members() const {
        std::shared_lock lock(room_users_mut);
        return room_users.size();
    }
};

struct User {
    mutable std::shared_mutex mut;
    std::string name;
    std::string address;
    std::string curr_room;
    int fd;

    User(const std::string& address, int fd): name(address), address(address), fd(fd) {}
    
    friend std::ostream& operator<<(std::ostream& o, const User& user) {
        o << "(" << user.name << "/" << user.address << "/" << user.fd << ")";
        return o;
    }
};

struct UserData {
    std::string name;
    std::string address;
    int fd;

    UserData() = default;
    UserData(const User& user): name(user.name), address(user.address), fd(user.fd) {}

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

    friend std::ostream& operator<<(std::ostream& o, const UserData& user) {
        o << "(" << user.name << "/" << user.address << "/" << user.fd << ")";
        return o;
    }
};

struct RoomData {
    std::string name;
    int num_members;

    RoomData() = default;
    RoomData(std::string_view name, int num_members): name(name), num_members(num_members) {}

    bool send(int socket) const {
        return send_string(socket, name)
            && send_i32(socket, num_members);
    }

    bool recv(int socket) {
        return recv_string(socket, name)
            && recv_i32(socket, num_members);
    }

    friend std::ostream& operator<<(std::ostream& o, const RoomData& room) {
        o << room.name << "(" << room.num_members << ")";
        return o;
    }
};

#endif