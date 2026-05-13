#ifndef UTILS_HPP
#define UTILS_HPP

#include <arpa/inet.h>

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

#endif