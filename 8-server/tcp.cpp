#include <algorithm>
#include <cerrno>
#include <iostream>
#include <memory>
#include <string>
#include <shared_mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "utils.hpp"
#include "serialize.hpp"
#include "types.hpp"

struct ControlCenter {
    mutable std::shared_mutex rooms_mut;
    std::unordered_map<std::string, std::unique_ptr<Room>> rooms;

    mutable std::shared_mutex users_mut;
    std::unordered_map<int, std::unique_ptr<User>> users;

    bool register_user(const std::string& address, int fd) {
        std::unique_lock lock(users_mut);
        if (users.find(fd) != users.end()) return false;
        users[fd] = std::make_unique<User>(address, fd);
        return true;
    }

    bool unregister_user(int fd) {
        std::unique_lock lock(users_mut);
        if (users.find(fd) == users.end()) return false;
        users.erase(fd);
        return true;
    }

    bool rename_user(int fd, std::string_view name) {
        {
            std::shared_lock lock(users_mut);
            if (users.find(fd) == users.end()) return false;
        }
        std::unique_lock lock(users[fd]->mut);
        users[fd]->name = name;
        return true;
    }

    const User& get_user(int fd) {
        std::shared_lock lock(users_mut);
        return *users[fd].get();
    }

    size_t get_num_users() const {
        return users.size();
    }

    ListRoomsResp list_rooms() {
        ListRoomsResp body;
        {
            std::shared_lock lock(rooms_mut);
            for (auto& p: rooms) {
                body.rooms.emplace_back(p.first, p.second->get_num_members());
            }
        }
        return body;
    }

    ListRoomMembersResp list_room_members(std::string& roomname) {
        ListRoomMembersResp body;
        Room* room = nullptr;
        {
            std::shared_lock lock(rooms_mut);
            if (rooms.find(roomname) == rooms.end()) {
                return body;
            };
            room = rooms[roomname].get();
        }
        std::shared_lock lock(room->room_users_mut);
        for (int fd: room->room_users) {
            std::shared_lock lock(users[fd]->mut);
            body.members.emplace_back(get_user(fd));
        }
        return body;
    }

    bool get_room(std::string& roomname, Room*& room) {
        std::shared_lock lock(rooms_mut);
        if (rooms.find(roomname) == rooms.end()) return false;
        room = rooms[roomname].get();
        return true;
    }

    bool join_room(int clientSocket, std::string& roomname) {
        Room* room;
        if (!get_room(roomname, room)) return false;

        auto& room_users = room->room_users;
        std::unique_lock lock2(room->room_users_mut);
        if (room_users.find(clientSocket) != room_users.end()) return false;
        room_users.insert(clientSocket);
        return true;
    }

    bool leave_room(int clientSocket, std::string& roomname) {
        Room* room;
        if (!get_room(roomname, room)) return false;

        auto& room_users = room->room_users;
        std::unique_lock lock2(room->room_users_mut);
        if (room_users.find(clientSocket) == room_users.end()) return false;
        room_users.erase(clientSocket);
        return true;
    }

    bool create_room(std::string& roomname) {
        Room* room;
        if (get_room(roomname, room)) return false;

        std::unique_lock lock(rooms_mut);
        rooms[roomname] = std::make_unique<Room>(roomname);
        return true;
    }

    bool broadcast_msg(std::string& roomname, int clientSocket, std::string_view msg) {
        Room* room;
        if (!get_room(roomname, room)) return false;

        RoomMessageResp body(get_user(clientSocket), msg);
        auto& room_users = room->room_users;
        std::shared_lock lock(rooms[roomname]->room_users_mut);
        for (int fd: room_users) {
            if (!send_body(fd, body)) return false;
        }
        return true;
    }
};

ControlCenter center;

int gateway_thread(in_port_t tcp_port, in_port_t gateway_port) {
    std::cout << "Started gateway thread with tcp_port=" << tcp_port << " and gateway_port=" << gateway_port << "\n";
    int gatewaySocket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in socketAddress = create_address(gateway_port);
    if (bind(gatewaySocket, reinterpret_cast<const sockaddr*>(&socketAddress), sizeof(socketAddress)) != 0) {
        std::cerr << "Failed to bind: " << errno << "\n";
        close(gatewaySocket);
        return 1;
    }

    in_port_t _tcp_port = htons(tcp_port);
    constexpr size_t buf_size = 1024;
    char buffer[buf_size + 1] = {0};
    sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    while (true) {
        size_t n = recvfrom(gatewaySocket, buffer, buf_size, MSG_TRUNC, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (n <= 0) continue;
        n = sendto(gatewaySocket, &_tcp_port, sizeof(_tcp_port), 0, reinterpret_cast<sockaddr*>(&clientAddr), clientAddrSize);
        if (n <= 0) {
            std::string clientip = get_str_address(clientAddr);
            std::cout << "Received msg from " << clientip << " but fail to respond.\n";
        }
    }
}

int connection_thread(int clientSocket, sockaddr_in clientAddr) {
    std::string address = get_str_address(clientAddr);
    if (!center.register_user(address, clientSocket)) {
        std::cout << "Failed to registered user " << address << "\n";
        close(clientSocket);
        return -1;
    };

    std::cout << "Thread started for " << center.get_user(clientSocket) << "\n";

    while (true) {
        int opscode;
        if (!recv_i32(clientSocket, opscode)) {
            std::cout << "Failed to receive body\n";
            break;
        }
        Ops ops = static_cast<Ops>(opscode);
        switch (ops) {
            case Ops::Join: {
                JoinBody body;
                if (!body.recv(clientSocket)) break;
                bool success = center.join_room(clientSocket, body.roomname);
                JoinResp resp{body.roomname, success};
                if (!send_body(clientSocket, resp)) break;
                continue;
            }
            case Ops::Leave: {
                LeaveBody body;
                if (!body.recv(clientSocket)) break;
                bool success = center.leave_room(clientSocket, body.roomname);
                LeaveResp resp{body.roomname, success};
                if (!send_body(clientSocket, resp)) break;
                continue;
            }
            case Ops::ListRoomMembers: {
                ListRoomMembersBody body;
                if (!body.recv(clientSocket)) break;
                ListRoomMembersResp resp = center.list_room_members(body.roomname);
                if (!send_body(clientSocket, resp)) break;
                continue;
            }
            case Ops::ListRooms: {
                ListRoomsBody body;
                if (!body.recv(clientSocket)) break;
                ListRoomsResp resp = center.list_rooms();
                if (!send_body(clientSocket, resp)) break;
                continue;
            }
            case Ops::CreateRoom: {
                CreateRoomBody body;
                if (!body.recv(clientSocket)) break;
                bool success = center.create_room(body.room_name);
                std::cout << "Created room " << success << "\n";
                CreateRoomResp resp{body.room_name, success};
                if (!send_body(clientSocket, resp)) break;
                continue;
            }
            case Ops::RoomMessage: {
                RoomMessageBody body;
                if (!body.recv(clientSocket)) break;
                center.broadcast_msg(body.room_name, clientSocket, body.msg);
                continue;
            }
            default:
                break;
        }
        break;
    }
    close(clientSocket);
    std::cout << "Thread closed for " << center.get_user(clientSocket) << "\n";
    return 0;
}

int main() {
    constexpr in_port_t gateway_port = 12345;
    constexpr in_port_t tcp_port = 8080;
    std::thread _gateway_thread(gateway_thread, 8080, 12345);
    _gateway_thread.detach();

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    int reuse_addr = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

    sockaddr_in socketAddress = create_address(tcp_port, INADDR_ANY);
    if (bind(serverSocket, reinterpret_cast<const sockaddr*>(&socketAddress), sizeof(socketAddress)) != 0) {
        std::cerr << "Failed to bind: " << errno << "\n";
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) != 0) {
        std::cout << "Failed to listen\n";
        close(serverSocket);
        return -1;
    }
    std::cout << "Listening for connections\n";

    while (true) {
        sockaddr_in clientAddr;
        socklen_t socklen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &socklen);
        if (clientSocket < 0) {
            std::cerr << "Accept failed: " << errno << "\n";
            continue;
        }

        std::thread(connection_thread, clientSocket, clientAddr).detach();
        std::cout << "Connection received\n";
    }

    close(serverSocket);
    return 0;
}