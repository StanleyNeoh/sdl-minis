#ifndef UTILS_HPP
#define UTILS_HPP

#include <arpa/inet.h>
#include <charconv>
#include <string>
#include <string_view>

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

#endif