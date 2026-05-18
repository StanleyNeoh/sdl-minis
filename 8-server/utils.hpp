#ifndef UTILS_HPP
#define UTILS_HPP

#include <charconv>
#include <string>
#include <string_view>
#include "platform_socket.hpp"

std::string_view trim_leading_spaces(std::string_view text) {
    std::size_t start = 0;
    while (start < text.size() && text[start] == ' ') {
        ++start;
    }

    return text.substr(start);
}

sockaddr_in create_address(in_port_t port, in_addr_t addr = INADDR_ANY) {
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port); // converts to network byte order
    serverAddress.sin_addr.s_addr = addr; // Socket listens to all available IPs (Main TCP socket to start handshake)
    return serverAddress;
}

std::string get_str_address(const sockaddr_in& clientAddr) {
    char clientIp[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
    std::string addr = std::string(clientIp) + ":" + std::to_string(ntohs(clientAddr.sin_port));
    return addr;
}

bool parse_command(std::string_view buffer, std::string_view& cmd, std::string_view& rest) {
    buffer = trim_leading_spaces(buffer);
    if (buffer.empty()) return false;
    if (buffer[0] != '/') return false;
    std::size_t command_start = 0;
    buffer = buffer.substr(1);
    if (buffer.empty()) return false;
    if (buffer[0] == ' ') return false;
    std::size_t i = buffer.find(' ');
    if (i == std::string_view::npos) {
        cmd = buffer.substr(command_start);
        rest = "";
    } else {
        cmd = buffer.substr(command_start, i);
        rest = buffer.substr(i + 1);
    }
    return true;
}

int parse_args(std::string_view buffer) {
    return 0;
}

template <typename T, typename ...Args>
int parse_args(std::string_view buffer, T& arg, Args&... args) {
    buffer = trim_leading_spaces(buffer);
    if (buffer.empty()) {
        return 0;
    }

    std::size_t i = buffer.find(' ');
    if (i == std::string_view::npos) {
        arg = buffer;
        return 1;
    } else {
        arg = buffer.substr(0, i);
        buffer = buffer.substr(i + 1);
        return 1 + parse_args(buffer, args...);
    }
}

bool to_int(std::string_view args, int& room_id) {
    args = trim_leading_spaces(args);

    if (args.empty()) {
        return false;
    }

    const char* begin = args.data();
    const char* end = args.data() + args.size();
    auto [ptr, ec] = std::from_chars(begin, end, room_id);
    return ec == std::errc() && ptr == end;
}

bool find_servers(sockaddr_in& serverAddress, int gateway_port) {
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Failed to create UDP socket: " << socket_error() << "\n";
        return false;
    }

    {
        int is_broadcast = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&is_broadcast), sizeof(is_broadcast)) != 0) {
            std::cerr << "Failed to set SO_BROADCAST: " << socket_error() << "\n";
            close(clientSocket);
            return false;
        }
    }

    {
        sockaddr_in broadcastAddress = create_address(gateway_port, INADDR_BROADCAST);
        static std::string_view buffer = "hi";
        ssize_t n = sendto(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0, reinterpret_cast<sockaddr*>(&broadcastAddress), sizeof(broadcastAddress));
        std::cout << "Sending UDP n=" << n << " to broadcast port " << gateway_port << ". Error: " << socket_error() <<"\n";
    }

    in_port_t port;
    socklen_t addressSize = sizeof(serverAddress);
    ssize_t n = recvfrom(clientSocket, reinterpret_cast<char*>(&port), sizeof(port), 0, reinterpret_cast<sockaddr*>(&serverAddress), &addressSize);
    if (n <= 0) {
        std::cerr << "Failed to receive UDP gateway response: " << socket_error() << "\n";
        close(clientSocket);
        return false;
    }
    serverAddress.sin_port = port; // suppose to store in network order. Not ntohs required
    close(clientSocket);
    return true;
}

#endif