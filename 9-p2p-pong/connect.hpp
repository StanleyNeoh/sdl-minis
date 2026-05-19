#ifndef CONNECT_HPP
#define CONNECT_HPP

#include <iostream>
#include <thread>
#include "locdata.hpp"
#include "platform_socket.hpp"

void connection_thread(int clientSocket, bool is_master) {
    curr_state.store(GameState_InGame, std::memory_order_release);
    int i = 0;
    int alive = 1;
    if (is_master) {
        ssize_t nbytes = send(clientSocket, &alive, sizeof(alive), 0);
        std::cout << "[Connection] Master sending first: " << nbytes << "\n";
    }

    while (curr_state.load(std::memory_order_acquire) == GameState_InGame) {
        std::cout << "Game on " << i << "\n";
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
    close(clientSocket);
};

int invite_user(const LocData& locdata) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cout << "Failed to create socket\n";
        return -1;
    }
    {
        int reuse_addr = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse_addr), sizeof(reuse_addr))) {
            std::cout << "Failed to set socket to be reusable\n";
            close(clientSocket);
            return -1;
        }
    }
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(locdata.port);
    serverAddress.sin_addr.s_addr = locdata.address;

    if (connect(clientSocket, reinterpret_cast<const sockaddr*>(&serverAddress), sizeof(serverAddress))) {
        std::cout << "Failed to connect to tcp server: " << errno << "\n";
        close(clientSocket);
        return -1;
    }
    std::thread _conn_thread(connection_thread, clientSocket, false);
    _conn_thread.detach();
    return -1;
}

int tcp_server_thread(int tcpPort) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cout << "Failed to create socket\n";
        return -1;
    }
    {
        int reuse_addr = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse_addr), sizeof(reuse_addr))) {
            std::cout << "Failed to set socket to be reusable\n";
            close(serverSocket);
            return -1;
        }
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(tcpPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(serverSocket, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr))) {
        std::cout << "Failed to bind sever to port\n";
        close(serverSocket);
        return -1;
    }
    
    if (listen(serverSocket, 1)) {
        std::cout << "Failed to prepare serversocket to listen\n";
        close(serverSocket);
        return -1;
    }
    std::cout << "Listening for connections\n";
    curr_state.store(GameState_Available, std::memory_order_relaxed);
    while (is_running.load(std::memory_order_relaxed)) {
        sockaddr_in clientAddr;
        socklen_t socklen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &socklen);
        if (clientSocket < 0) {
            std::cout << "Accept failed: " << errno << "\n";
            continue;
        }
        connection_thread(clientSocket, true);
    }
    close(serverSocket);
    return 0;
}

#endif
