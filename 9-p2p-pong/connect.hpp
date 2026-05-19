#ifndef CONNECT_HPP
#define CONNECT_HPP

#include <iostream>
#include <thread>
#include "locdata.hpp"
#include "platform_socket.hpp"

void connection_thread(SocketResource clientSocket, bool is_master) {
    if (!clientSocket.is_available()) {
        std::cout << "[Connection] clientsocket was not initialised\n";
        return;
    }

    curr_state.store(GameState_InGame, std::memory_order_release);
    int alive = 1;
    if (is_master) {
        ssize_t nbytes = send(clientSocket, &alive, sizeof(alive), 0);
        std::cout << "[Connection] Master sending first: " << nbytes << "\n";
    }

    while (curr_state.load(std::memory_order_acquire) == GameState_InGame) {
        ssize_t nbytes = recv(clientSocket, &alive, sizeof(alive), 0);
        if (nbytes == 0) {
            std::cout << "[Connection] Socket is dead. breaking\n";
            break;
        } else {
            std::cout << "[Connection] Recved " << nbytes << "\n";
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
        nbytes = send(clientSocket, &alive, sizeof(alive), 0);
        std::cout << "[Connection] Sending next...: " << nbytes << "\n";
    }
    curr_state.store(GameState_Available, std::memory_order_release);
    std::cout << "[Connection] Closing tcp\n";
};

int invite_user(const LocData& locdata) {
    SocketResource socketResource(AF_INET, SOCK_STREAM, 0);
    if (!socketResource.is_available()) {
        std::cout << "[Invite] Failed to create socket " << socket_error() << "\n";
        return -1;
    }

    if (socketResource.setsockopt(SO_REUSEADDR, 1)) {
        std::cout << "[Invite] Failed to set socket to be reusable\n";
        return -1;
    }

    sockaddr_in serverAddress = create_sockaddr(locdata.address, locdata.port);
    if (socketResource.connect(serverAddress)) {
        std::cout << "[Invite] Failed to connect to tcp server: " << errno << "\n";
        return -1;
    }

    std::thread _conn_thread(connection_thread, std::move(socketResource), false);
    _conn_thread.detach();
    return -1;
}

int tcp_server_thread(int tcpPort) {
    SocketResource socketResource(AF_INET, SOCK_STREAM, 0);
    if (!socketResource.is_available()) {
        std::cout << "[TCP] Failed to create socket: " << socket_error() << "\n";
        return -1;
    }
    if (socketResource.setsockopt(SO_REUSEADDR, 1)) {
        std::cout << "[TCP] Failed to set socket to be reusable\n";
        return -1;
    }

    sockaddr_in serverAddr = create_sockaddr(INADDR_ANY, tcpPort);
    if (socketResource.bind(serverAddr)) {
        std::cout << "[TCP] Failed to bind sever to port\n";
        return -1;
    }
    
    if (socketResource.listen(1)) {
        std::cout << "[TCP] Failed to prepare serversocket to listen\n";
        return -1;
    }
    std::cout << "[TCP] Listening for connections\n";

    curr_state.store(GameState_Available, std::memory_order_relaxed);
    while (is_running.load(std::memory_order_relaxed)) {
        sockaddr_in clientAddr;
        SocketResource clientResource = socketResource.accept(clientAddr);
        if (!clientResource.is_available()) {
            std::cout << "[TCP] Accept failed: " << errno << "\n";
            continue;
        }
        connection_thread(std::move(clientResource), true);
    }
    return 0;
}

#endif
