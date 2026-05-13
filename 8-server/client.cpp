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