
#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <cstdint>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>

bool recv_exact(int socket, void* buffer, std::size_t size) {
    auto* bytes = static_cast<char*>(buffer);
    std::size_t received = 0;
    while (received < size) {
        ssize_t nbytes = recv(socket, bytes + received, size - received, 0);
        if (nbytes <= 0) {
            return false;
        }
        received += static_cast<std::size_t>(nbytes);
    }
    return true;
}

bool send_exact(int socket, const void* buffer, std::size_t size) {
    const auto* bytes = static_cast<const char*>(buffer);
    std::size_t sent = 0;
    while (sent < size) {
        ssize_t nbytes = send(socket, bytes + sent, size - sent, 0);
        if (nbytes <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(nbytes);
    }
    return true;
}

bool recv_i32(int socket, int& out) {
    std::int32_t value = 0;
    if (!recv_exact(socket, &value, sizeof(value))) {
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


bool send_i32(int socket, int value) {
    std::int32_t encoded = htonl(static_cast<std::int32_t>(value));
    return send_exact(socket, &encoded, sizeof(encoded));
}

bool recv_string(int socket, int size, std::string& out) {
    if (size < 0) {
        return false;
    }

    out.resize(static_cast<std::size_t>(size));
    return size == 0 || recv_exact(socket, out.data(), static_cast<std::size_t>(size));
}


#endif