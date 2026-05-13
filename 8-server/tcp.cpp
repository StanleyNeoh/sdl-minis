#include <algorithm>
#include <cerrno>
#include <iostream>
#include <memory>
#include <shared_mutex>
#include <string>
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
struct Users {
    std::shared_mutex users_mut;
    std::unordered_map<int, std::unique_ptr<User>> users;
    std::unordered_map<int, std::shared_mutex> user_muts;

    bool register_user(const std::string& address, int clientSocket) {
        std::unique_lock lock(users_mut);
        if (users.find(clientSocket) != users.end()) return false;
        users[clientSocket] = std::make_unique<User>(address, clientSocket);
        user_muts.try_emplace(clientSocket);
        return true;
    }

    bool unregister_user(int clientSocket) {
        std::unique_lock lock(users_mut);
        if (users.find(clientSocket) == users.end()) return false;
        users.erase(clientSocket);
        user_muts.erase(clientSocket);
        return true;
    }

    bool rename_user(int clientSocket, std::string_view name) {
        {
            std::shared_lock lock(users_mut);
            if (users.find(clientSocket) == users.end()) return false;
        }
        std::shared_lock lock(user_muts[clientSocket]);
        users[clientSocket]->name = name;
        return true;
    }

    User& get(int clientSocket) {
        return *users[clientSocket];
    }

    size_t size() const {
        return users.size();
    }
};

Users users;
struct Room {
    std::string roomname;
    std::shared_mutex users_mut;
    std::unordered_set<int> room_users;

    Room(std::string& roomname): roomname(roomname) {}

    bool add_user(int clientSocket) {
        {
            std::shared_lock _lock(users_mut);
            if (room_users.find(clientSocket) != room_users.end()) return false;
        }
        {
            std::unique_lock _lock(users_mut);
            room_users.insert(clientSocket);
        }
        return true;
    }

    bool remove_user(int clientSocket) {
        std::unique_lock _lock(users_mut);
        return room_users.erase(clientSocket) != 0;
    }

    ListRoomMembersResp list_room_members() {
        std::shared_lock lock(users_mut);
        ListRoomMembersResp body;
        for (int p: room_users) {
            body.members.push_back(users.get(p));
        }
        return body;
    }

    bool broadcast_msg(const User& user, std::string_view msg) {
        RoomMessageResp body(user, msg);
        for (int p: room_users) {
            if (!body.send(p)) return false;;
        }
        return true;
    }

    int get_num_members() {
        std::shared_lock lock(users_mut);
        return users.size();
    }
};

struct Rooms {
    std::shared_mutex rooms_mut;
    std::unordered_map<std::string, std::unique_ptr<Room>> rooms;

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
        return room->list_room_members();
    }

    bool join_room(int clientSocket, std::string& roomname) {
        std::shared_lock lock(rooms_mut);
        return rooms[roomname]->add_user(clientSocket);
    }

    bool leave_room(int clientSocket, std::string& roomname) {
        std::shared_lock lock(rooms_mut);
        return rooms[roomname]->remove_user(clientSocket);
    }

    bool create_room(std::string& roomname) {
        std::unique_lock _lock(rooms_mut);
        if (rooms.find(roomname) != rooms.end()) {
            return false;
        }
        rooms[roomname] = std::make_unique<Room>(roomname);
        return true;
    }

    bool broadcast_msg(std::string& roomname, const User& user, std::string_view msg) {
        std::shared_lock _lock(rooms_mut);
        if (rooms.find(roomname) != rooms.end()) {
            return false;
        }
        return rooms[roomname]->broadcast_msg(user, msg);
    }
};

Rooms rooms;

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
    if (!users.register_user(address, clientSocket)) {
        std::cout << "Failed to registered user " << address << "\n";
        close(clientSocket);
        return -1;
    };

    std::cout << "Thread started for " << address << "\n";
    while (true) {
        int opscode;
        if (!recv_i32(clientSocket, opscode)) {
            std::cout << "Failed to receive body\n";
            continue;
        }
        Ops ops = static_cast<Ops>(opscode);
        switch (ops) {
            case Ops::Join: {
                JoinBody body;
                body.recv(clientSocket);
                bool success = rooms.join_room(clientSocket, body.roomname);
                JoinResp resp{body.roomname, success};
                resp.send(clientSocket);
                continue;
            }
            case Ops::Leave: {
                LeaveBody body;
                body.recv(clientSocket);
                bool success = rooms.leave_room(clientSocket, body.roomname);
                LeaveResp resp{body.roomname, success};
                resp.send(clientSocket);
                continue;
            }
            case Ops::ListRoomMembers: {
                ListRoomMembersBody body;
                body.recv(clientSocket);
                ListRoomMembersResp resp = rooms.list_room_members(body.roomname);
                resp.send(clientSocket);
                continue;
            }
            case Ops::ListRooms: {
                ListRoomsBody body;
                body.recv(clientSocket);
                ListRoomsResp resp = rooms.list_rooms();
                resp.send(clientSocket);
                continue;
            }
            case Ops::CreateRoom: {
                CreateRoomBody body;
                body.recv(clientSocket);
                bool success = rooms.create_room(body.room_name);
                CreateRoomResp resp{body.room_name, success};
                resp.send(clientSocket);
                continue;
            }
            case Ops::RoomMessage: {
                RoomMessageBody body;
                body.recv(clientSocket);
                rooms.broadcast_msg(body.room_name, users.get(clientSocket), body.msg);
                continue;
            }
        }
        break;
    }
    close(clientSocket);
    std::cout << "Thread closed for " << users.get(clientSocket) << "\n";
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
    std::cout << "Listening for userections\n";

    while (true) {
        sockaddr_in clientAddr;
        socklen_t socklen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &socklen);
        if (clientSocket < 0) {
            std::cerr << "Accept failed: " << errno << "\n";
            continue;
        }

        std::thread(connection_thread, clientSocket, clientAddr).detach();
        std::cout << "userection received\n";
    }

    close(serverSocket);
    return 0;
}