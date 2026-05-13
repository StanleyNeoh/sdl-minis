#include <cerrno>
#include <thread>
#include <charconv>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "serialize.hpp"
#include "utils.hpp"

int current_room = -1;

bool parse_room_id(std::string_view args, int& room_id) {
    args = trim_leading_spaces(args);

    if (args.empty()) {
        return false;
    }

    const char* begin = args.data();
    const char* end = args.data() + args.size();
    auto [ptr, ec] = std::from_chars(begin, end, room_id);
    return ec == std::errc() && ptr == end;
}

bool send_command(int clientSocket, const std::string& buffer) {
    std::string_view input = trim_leading_spaces(buffer);
    if (input.empty()) {
        return false;
    }

    std::size_t split = input.find(' ');
    std::string_view command = input.substr(0, split);
    std::string_view args = split == std::string_view::npos ? std::string_view{} : input.substr(split + 1);

    if (command == "reg") {
        int room_id = 0;
        if (!parse_room_id(args, room_id)) {
            return false;
        }

        JoinBody body{room_id};
        return send_body(clientSocket, body);
    } else if (command == "join") {
        int room_id = 0;
        if (!parse_room_id(args, room_id)) {
            return false;
        }
        current_room = room_id;
        std::cout << "Joined " << current_room << "\n";
        return true;
    } else if (command == "leave") {
        int room_id = 0;
        if (!parse_room_id(args, room_id)) {
            return false;
        }

        LeaveBody body{room_id};
        return send_body(clientSocket, body);
    } else if (command == "members") {
        int room_id = 0;
        if (!parse_room_id(args, room_id)) {
            return false;
        }

        ListRoomMembersBody body{room_id};
        return send_body(clientSocket, body);
    } else if (command == "list") {
        if (!trim_leading_spaces(args).empty()) {
            return false;
        }
        return send_body(clientSocket, ListRoomsBody{});
    } else if (command == "create") {
        std::string roomname(trim_leading_spaces(args));

        if (roomname.empty()) {
            return false;
        }

        CreateRoomBody body{roomname};
        return send_body(clientSocket, body);
    } else {
        if (current_room < 0) {
            return false;
        }

        RoomMessageBody body{current_room, std::string(input)};
        return send_body(clientSocket, body);
    }

    return false;
}

void recv_handler(int clientSocket) {
    Body body;
    while (true) {
        if (!recv_body(clientSocket, body)) {
            std::cout << "Failed to receive body\n";
            break;
        }

        switch(body.ops) {
        case Ops::ServerMessage: {
            std::cout << body.server_msg.msg << "\n";
            continue;
        }
        }
        break;
    }
}

bool find_servers(sockaddr_in& serverAddress, int gateway_port) {
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Failed to create UDP socket. Errno: " << errno << "\n";
        return false;
    }

    {
        int is_broadcast = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_BROADCAST, &is_broadcast, sizeof(is_broadcast)) != 0) {
            std::cerr << "Failed to set sockopt. Errno: " << errno << "\n";
            close(clientSocket);
            return false;
        }
    }

    {
        sockaddr_in broadcastAddress = create_address(gateway_port, INADDR_BROADCAST);
        static std::string_view buffer = "hi";
        ssize_t n = sendto(clientSocket, &buffer, buffer.size(), 0, reinterpret_cast<sockaddr*>(&broadcastAddress), sizeof(broadcastAddress));
        std::cout << "Sending UDP n=" << n << " to " << get_str_address(serverAddress) << ". ErrNo: " << errno <<"\n";
    }

    in_port_t port;
    socklen_t addressSize = sizeof(serverAddress);
    ssize_t n = recvfrom(clientSocket, &port, sizeof(port), MSG_TRUNC, reinterpret_cast<sockaddr*>(&serverAddress), &addressSize);
    serverAddress.sin_port = port; // suppose to store in network order. Not ntohs required
    return true;
}

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    std::cout << "Finding servers with gateway port 12345\n";
    sockaddr_in serverAddress; 
    if (!find_servers(serverAddress, 12345)) {
        std::cerr << "Unable to find server\n";
        return 1;
    };

    std::cout << "Found server address" << get_str_address(serverAddress) << "\n";
    if (connect(clientSocket, reinterpret_cast<const sockaddr*>(&serverAddress), sizeof(serverAddress)) != 0) {
        std::cerr << "Failed to connect: " << errno << "\n";
        close(clientSocket);
        return 1;
    };

    std::thread _recv_handler(recv_handler, clientSocket);
    _recv_handler.detach();

    std::string buffer;
    Body resp_body;
    while (std::getline(std::cin, buffer)) {
        if (!send_command(clientSocket, buffer)) {
            std::cout << "Supported commands: join <room id>, leave <room id>, members <room id>, list, create <room name>\n";
            continue;
        }
    }

    close(clientSocket);
    return 0;
}