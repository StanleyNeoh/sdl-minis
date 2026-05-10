#include <vector>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include "utils.hpp"

struct Conn {
    int conn_id;
    int conn_socket;
};

struct Message {
    int conn_id;
    std::string msg;
};

struct Room {
    std::string roomname;
    Room(const std::string roomname): roomname(roomname) {}
};

std::shared_mutex rooms_mut;
std::vector<std::unique_ptr<Room>> rooms;
bool room_name_for_id(int room_id, std::string& roomname) {
    std::shared_lock lock(rooms_mut);
    if (room_id < 0 || static_cast<std::size_t>(room_id) >= rooms.size()) {
        return false;
    }

    roomname = rooms[static_cast<std::size_t>(room_id)]->roomname;
    return true;
}

int thread_handler(int clientSocket, sockaddr_in clientAddr) {
    std::string clientIp = client_ip(clientAddr);
    std::cout << "Thread started for " << clientIp << "\n";
    while (true) {
        Ops opcode = Ops::ListRooms;
        int op1 = 0;
        int op2 = 0;
        std::string aux;
        RecvCode recv_code = read_cmd(clientSocket, opcode, op1, op2, aux);
        if (recv_code == RecvCode::Closed) {
            break;
        }
        if (recv_code == RecvCode::Invalid) {
            std::cout << "Invalid command from " << clientIp << "\n";
            break;
        }

        std::cout << static_cast<int>(opcode) << " " << op1 << " " << op2 << " " << aux << "\n";
        if (opcode == Ops::Message) {
            std::string roomname;
            if (!room_name_for_id(op1, roomname)) {
                std::cout << "Invalid room id " << op1 << " from " << clientIp << "\n";
                continue;
            }
            std::cout << "Message from " << clientIp << " to room " << roomname << ": " << aux << "\n";
        } else if (opcode == Ops::Join) {
            std::string roomname;
            if (!room_name_for_id(op1, roomname)) {
                std::cout << "Invalid room id " << op1 << " from " << clientIp << "\n";
                continue;
            }
            std::cout << "Join from " << clientIp << " to room " << roomname << "\n";
        } else if (opcode == Ops::Leave) {
            std::string roomname;
            if (!room_name_for_id(op1, roomname)) {
                std::cout << "Invalid room id " << op1 << " from " << clientIp << "\n";
                continue;
            }
            std::cout << "Leave from " << clientIp << " to room " << roomname << "\n";
        } else if (opcode == Ops::ListRooms) {
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
            if (!send_i32(clientSocket, static_cast<int>(Ops::ListRooms))
                || !send_i32(clientSocket, static_cast<int>(body.size()))
                || (!body.empty() && !send_exact(clientSocket, body.data(), body.size()))) {
                break;
            }
        } else if (opcode == Ops::CreateRoom) {
            int room_id = 0;
            {
                std::unique_lock _lock(rooms_mut);
                room_id = static_cast<int>(rooms.size());
                rooms.push_back(std::make_unique<Room>(aux));
            }

            if (!send_i32(clientSocket, static_cast<int>(Ops::CreateRoom))
                || !send_i32(clientSocket, room_id)) {
                break;
            }
        }
    }
    close(clientSocket);
    std::cout << "Thread closed for " << clientIp << "\n";
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