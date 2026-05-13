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
#include "utils.hpp"
#include "serialize.hpp"

struct Conn {
    int conn_socket;
    std::string client_address;

    Conn(int conn_socket, const std::string& client_address): conn_socket(conn_socket), client_address(client_address) {}

    friend std::ostream& operator<<(std::ostream& o, const Conn& conn) {
        o << "(" << conn.client_address << ":" << conn.conn_socket << ")";
        return o;
    }
};

struct Room {
    std::string roomname;
    std::shared_mutex conns_mut;
    std::vector<Conn> conns;

    Room(std::string_view roomname): roomname(roomname) {}

    bool add_conn(const Conn& conn) {
        {
            std::shared_lock _lock(conns_mut);
            for (auto& _conn: conns) {
                if (_conn.client_address == conn.client_address) return false;
            }
        }
        {
            std::unique_lock _lock(conns_mut);
            conns.push_back(conn);
        }

        return true;
    }

    bool remove_conn(const Conn& conn) {
        std::unique_lock _lock(conns_mut);
        for (auto& _conn: conns) {
            if (_conn.client_address == conn.client_address) {
                std::swap(_conn, conns.back());
                conns.pop_back();
                return true;
            }
        }
        return false;
    }

    std::string list_conns() {
        std::string body;
        std::shared_lock lock(conns_mut);
        for (std::size_t i = 0; i < conns.size(); ++i) {
            if (!body.empty()) {
                body += ",";
            }
            body += std::to_string(i) + ":" + conns[i].client_address;
        }
        return body;
    }


    bool broadcast_msg(const Conn& conn, std::string_view msg) {
        ServerMessageBody msgBody;
        msgBody.msg = conn.client_address + "@" + roomname + ": " + std::string(msg);
        for (auto& _conn: conns) {
            if (!msgBody.send(_conn.conn_socket)) {
                return false;
            };
        }
        return true;
    }
};

struct Rooms {
    std::shared_mutex rooms_mut;
    std::vector<std::unique_ptr<Room>> rooms;

    std::string list_rooms() {
        std::string body;
        {
            std::shared_lock lock(rooms_mut);
            for (std::size_t i = 0; i < rooms.size(); ++i) {
                if (!body.empty()) {
                    body += ",";
                }
                body += std::to_string(i) + ":" + rooms[i]->roomname;
            }
        }

        return body;
    }

    std::string list_room_members(int room_id) {
        std::shared_lock lock(rooms_mut);
        return rooms[room_id]->list_conns();
    }

    bool join_room(const Conn& conn, int room_id) {
        std::shared_lock lock(rooms_mut);
        return rooms[room_id]->add_conn(conn);
    }

    bool leave_room(const Conn& conn, int room_id) {
        std::shared_lock lock(rooms_mut);
        return rooms[room_id]->remove_conn(conn);
    }

    int create_room(std::string_view roomname) {
        std::unique_lock _lock(rooms_mut);
        int room_id = static_cast<int>(rooms.size());
        rooms.push_back(std::make_unique<Room>(roomname));
        return room_id;
    }

    bool room_name_for_id(int room_id, std::string& roomname) {
        std::shared_lock lock(rooms_mut);
        if (room_id < 0 || static_cast<std::size_t>(room_id) >= rooms.size()) {
            return false;
        }

        roomname += rooms[static_cast<std::size_t>(room_id)]->roomname;
        return true;
    }

    bool broadcast_msg(int room_id, const Conn& conn, std::string_view msg) {
        ServerMessageBody msgBody;
        Room* room = nullptr;
        {
            std::shared_lock _lock(rooms_mut);
            room = rooms[room_id].get();
        }
        {
            std::shared_lock _lock(room->conns_mut);
            msgBody.msg = conn.client_address + "@" + room->roomname + ": " + std::string(msg);
            for (auto& _conn: room->conns) {
                if (!send_body(_conn.conn_socket, msgBody)) {
                    return false;
                };
            }
            return true;
        }
    }
};

Rooms rooms;

bool send_text_response(int clientSocket, const std::string& message) {
    ServerMessageBody body;
    body.msg = message;
    return send_body(clientSocket, body);
}

int thread_handler(int clientSocket, sockaddr_in clientAddr) {
    Conn conn(clientSocket, get_client_address(clientAddr));
    std::cout << "Thread started for " << conn << "\n";
    Body body;
    while (true) {
        if (!recv_body(clientSocket, body)) {
            std::cout << "Failed to receive body\n";
            break;
        }

        switch (body.ops) {
        case Ops::Join: {
            int room_id = body.join_body.room_id;
            std::string response = "Registered room ";
            if (!rooms.room_name_for_id(room_id, response)) {
                std::cout << "Invalid room id " << room_id << " from " << conn << "\n";
                if (!send_text_response(clientSocket, "Invalid room")) break;
                continue;
            }
            if (!rooms.join_room(conn, room_id)) {
                if (!send_text_response(clientSocket, "Already joined room")) break;
                continue;
            }
            if (!send_text_response(clientSocket, response)) break;
            continue;
        }
        case Ops::Leave: {
            int room_id = body.leave_body.room_id;
            std::string response = "Left room ";
            if (!rooms.room_name_for_id(room_id, response)) {
                std::cout << "Invalid room id " << room_id << " from " << conn << "\n";
                if (!send_text_response(clientSocket, "Invalid room")) break;
                continue;
            }
            if (!rooms.leave_room(conn, room_id)) {
                if (!send_text_response(clientSocket, "Not in room")) break;
                continue;
            }
            if (!send_text_response(clientSocket, response)) break;
            continue;
        }
        case Ops::ListRooms: {
            std::string response = rooms.list_rooms();
            if (!send_text_response(clientSocket, response)) break;
            continue;
        }
        case Ops::ListRoomMembers: {
            int room_id = body.list_room_members_body.room_id;
            std::string roomname;
            if (!rooms.room_name_for_id(room_id, roomname)) {
                std::cout << "Invalid room id " << room_id << " from " << conn << "\n";
                if (!send_text_response(clientSocket, "Invalid room")) break;
                continue;
            }
            std::string response = rooms.list_room_members(room_id);
            if (!send_text_response(clientSocket, response)) break;
            continue;
        }
        case Ops::CreateRoom: {
            std::string_view room_name = body.create_room_body.room_name;
            int room_id = rooms.create_room(room_name);
            std::string response = "Created room " + std::string(room_name) + " with id " + std::to_string(room_id);
            if (!send_text_response(clientSocket, response)) break;
            continue;
        }
        case Ops::RoomMessage: {
            int room_id = body.room_msg_body.room_id;
            std::string_view msg = body.room_msg_body.msg;
            rooms.broadcast_msg(body.room_msg_body.room_id, conn, body.room_msg_body.msg);
            continue;
        }
        }
        break;
    }
    close(clientSocket);
    std::cout << "Thread closed for " << conn << "\n";
    return 0;
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    int reuse_addr = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

    sockaddr_in socketAddress = create_address();
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

        std::thread(thread_handler, clientSocket, clientAddr).detach();
        std::cout << "Connection received\n";
    }

    close(serverSocket);
    return 0;
}