#include <string>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "utils.hpp"

void list_rooms(std::string& out) {
    out.clear();
    append_i32(out, static_cast<int>(Ops::ListRooms));
}

void create_room(std::string& out, const std::string& roomname) {
    out.clear();
    append_i32(out, static_cast<int>(Ops::CreateRoom));
    append_i32(out, static_cast<int>(roomname.size()));
    out += roomname;
}

bool parse_command(const std::string& buffer, std::string& out) {
    if (buffer.empty()) {
        return false;
    }

    if (buffer[0] == '3') {
        list_rooms(out);
        return true;
    } else if (buffer[0] == '4') {
        std::string roomname;
        for (std::size_t i = 1; i < buffer.size(); ++i) {
            if (buffer[i] != ' ') {
                roomname = std::string(buffer.data() + i);
                break;
            }
        }

        if (roomname.empty()) {
            return false;
        }

        create_room(out, roomname);
        return true;
    }

    return false;
}

bool read_response(int clientSocket) {
    int opcode = 0;
    if (!recv_i32(clientSocket, opcode)) {
        return false;
    }

    if (opcode == static_cast<int>(Ops::ListRooms)) {
        int body_size = 0;
        std::string body;
        if (!recv_i32(clientSocket, body_size) || !recv_string(clientSocket, body_size, body)) {
            return false;
        }
        std::cout << "Rooms: " << body << "\n";
        return true;
    }

    if (opcode == static_cast<int>(Ops::CreateRoom)) {
        int room_id = 0;
        if (!recv_i32(clientSocket, room_id)) {
            return false;
        }
        std::cout << "Room ID: " << room_id << "\n";
        return true;
    }

    std::cerr << "Unknown opcode: " << opcode << "\n";
    return false;
}

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in serverAddress = create_address();
    if (connect(clientSocket, reinterpret_cast<const sockaddr*>(&serverAddress), sizeof(serverAddress)) != 0) {
        std::cerr << "Failed to connect: " << errno << "\n";
        close(clientSocket);
        return 1;
    };

    std::string buffer;
    std::string out;
    while (std::getline(std::cin, buffer)) {
        if (!parse_command(buffer, out)) {
            std::cout << "Supported commands: 3, 4 <room name>\n";
            continue;
        }

        if (!send_exact(clientSocket, out.data(), out.size())) {
            std::cerr << "Failed to send request\n";
            break;
        }

        if (!read_response(clientSocket)) {
            std::cerr << "Failed to read response\n";
            break;
        }
    }

    close(clientSocket);
    return 0;
}