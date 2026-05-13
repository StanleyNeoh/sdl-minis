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


std::string curr_room;

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

void recv_thread(int clientSocket) {
    while (true) {
        int opscode;
        if (!recv_i32(clientSocket, opscode)) {
            std::cout << "Failed to receive body\n";
            break;
        }
        Ops ops = static_cast<Ops>(opscode);
        switch(ops) {
            case Ops::ServerMessage: {
                ServerMessageBody body;
                body.recv(clientSocket);
                std::cout << "[Server] " << body.msg << "\n";
                continue;
            }
            case Ops::JoinResp: {
                JoinResp body;
                body.recv(clientSocket);
                curr_room = body.roomname;
                std::cout << "[Join Response] Joined " << body.roomname << " " << (body.success ? "Success" : "Failed") << "\n";
                continue;
            }
            case Ops::LeaveResp: {
                LeaveResp body;
                body.recv(clientSocket);
                curr_room.clear();
                std::cout << "[Leave Response] Left " << body.roomname << " " << (body.success ? "Success" : "Failed") << "\n";
                continue;
            }
            case Ops::ListRoomMembersResp: {
                ListRoomMembersResp body;
                body.recv(clientSocket);
                std::cout << "Members:\n";
                for (auto& p: body.members) {
                    std::cout << " - " << p.name << "(" << p.address << " / " << p.fd << ")\n";
                }
                std::cout << "\n";
            }
            case Ops::ListRoomsResp: {
                ListRoomsResp body;
                body.recv(clientSocket);
                std::cout << "Rooms:\n";
                for (auto& p: body.rooms) {
                    std::cout << " - " << p << "\n";
                }
                std::cout << "\n";
            }
            case Ops::CreateRoomResp: {
                CreateRoomResp body;
                body.recv(clientSocket);
                std::cout << "[Create Room Response] " << body.room_name << " creation " << (body.success ? "success" : "failed") << "\n";
            }
            case Ops::RoomMessageResp: {
                RoomMessageResp body;
                body.recv(clientSocket);
                std::cout << body.user << " === " << body.msg << "\n";
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

    std::thread _recv_thread(recv_thread, clientSocket);
    _recv_thread.detach();

    std::string buffer;
    while (std::getline(std::cin, buffer)) {
        std::string_view command;
        std::string_view rest;
        if (parse_command(buffer, command, rest)) {
            if (command == "/join") {
                std::string_view roomname;
                if (parse_args(rest, roomname) == 1) {
                    JoinBody body {std::string(roomname)};
                    send_body(clientSocket, body);
                } else {
                    std::cout << "Help: /join <room_name>\n";
                }
            } else if (command == "/leave") {
                std::string_view roomname;
                if (parse_args(rest, roomname) == 1) {
                    LeaveBody body{std::string(roomname)};
                    return send_body(clientSocket, body);
                } else {
                    std::cout << "Help: /leave <room_name>\n";
                }
            } else if (command == "/members") {
                std::string_view roomname;
                if (parse_args(rest, roomname) == 1) {
                    int room_id = 0;
                    ListRoomMembersBody body{std::string(roomname)};
                    send_body(clientSocket, body);
                } else {
                    std::cout << "Help: /members <room_id>\n";
                }
            } else if (command == "/list") {
                send_body(clientSocket, ListRoomsBody{});
            } else if (command == "/create") {
                std::string_view roomname;
                if (parse_args(rest, roomname) == 1) {
                    CreateRoomBody body{std::string(roomname)};
                    send_body(clientSocket, body);
                } else {
                    std::cout << "Help: /create <room_id>\n";
                }
            }
        } else {
            if (curr_room.empty()) {
                std::cout << "[Error] Not in any room.\n";
            } else {
                RoomMessageBody body{curr_room, std::string(buffer)};
                send_body(clientSocket, body);
            }
        }
        return false;
    }

    close(clientSocket);
    return 0;
}