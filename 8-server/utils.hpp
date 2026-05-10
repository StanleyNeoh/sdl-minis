#ifndef UTILS_HPP
#define UTILS_HPP

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

enum struct Ops: int {
    Message = 0,
    Join = 1,
    Leave = 2,
    ListRooms = 3,
    CreateRoom = 4,
};

enum struct RecvCode {
    Ok,
    Closed,
    Invalid,
};

bool recv_exact(int clientSocket, void* buffer, std::size_t size) {
    auto* bytes = static_cast<char*>(buffer);
    std::size_t received = 0;
    while (received < size) {
        ssize_t nbytes = recv(clientSocket, bytes + received, size - received, 0);
        if (nbytes <= 0) {
            return false;
        }
        received += static_cast<std::size_t>(nbytes);
    }
    return true;
}

bool send_exact(int clientSocket, const void* buffer, std::size_t size) {
    const auto* bytes = static_cast<const char*>(buffer);
    std::size_t sent = 0;
    while (sent < size) {
        ssize_t nbytes = send(clientSocket, bytes + sent, size - sent, 0);
        if (nbytes <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(nbytes);
    }
    return true;
}

bool recv_i32(int clientSocket, int& out) {
    std::int32_t value = 0;
    if (!recv_exact(clientSocket, &value, sizeof(value))) {
        return false;
    }
    out = ntohl(value);
    return true;
}

void append_i32(std::string& out, int value) {
    std::int32_t encoded = htonl(static_cast<std::int32_t>(value));
    std::size_t old_size = out.size();
    out.resize(old_size + sizeof(encoded));
    std::memcpy(out.data() + old_size, &encoded, sizeof(encoded));
}


bool send_i32(int clientSocket, int value) {
    std::int32_t encoded = htonl(static_cast<std::int32_t>(value));
    return send_exact(clientSocket, &encoded, sizeof(encoded));
}

bool recv_string(int clientSocket, int size, std::string& out) {
    if (size < 0) {
        return false;
    }

    out.resize(static_cast<std::size_t>(size));
    return size == 0 || recv_exact(clientSocket, out.data(), static_cast<std::size_t>(size));
}

RecvCode read_cmd(
    int clientSocket, 
    Ops& opcode,
    int& op1,
    int& op2,
    std::string& aux
) {
    op1 = 0;
    op2 = 0;
    aux.clear();

    int opcode_value = 0;
    if (!recv_i32(clientSocket, opcode_value)) {
        return RecvCode::Closed;
    }

    switch (opcode_value) {
        case static_cast<int>(Ops::Message):
            opcode = Ops::Message;
            if (!recv_i32(clientSocket, op1) || !recv_i32(clientSocket, op2)) {
                return RecvCode::Closed;
            }
            if (!recv_string(clientSocket, op2, aux)) {
                return RecvCode::Invalid;
            }
            break;
        case static_cast<int>(Ops::Join):
            opcode = Ops::Join;
            if (!recv_i32(clientSocket, op1)) {
                return RecvCode::Closed;
            }
            break;
        case static_cast<int>(Ops::Leave):
            opcode = Ops::Leave;
            if (!recv_i32(clientSocket, op1)) {
                return RecvCode::Closed;
            }
            break;
        case static_cast<int>(Ops::ListRooms):
            opcode = Ops::ListRooms;
            break;
        case static_cast<int>(Ops::CreateRoom):
            opcode = Ops::CreateRoom;
            if (!recv_i32(clientSocket, op1)) {
                return RecvCode::Closed;
            }
            if (op1 < 0 || !recv_string(clientSocket, op1, aux)) {
                return RecvCode::Invalid;
            }
            break;
        default:
            return RecvCode::Invalid;
    }

    return RecvCode::Ok;
}

sockaddr_in create_address(int16_t port = 8080) {
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port); // converts to network byte order
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Socket listens to all available IPs (Main TCP socket to start handshake)
    return serverAddress;
}

std::string client_ip(const sockaddr_in& clientAddr) {
    char clientIp[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
    std::string addr = std::string(clientIp) + ":" + std::to_string(ntohs(clientAddr.sin_port));
    return addr;
}

#endif