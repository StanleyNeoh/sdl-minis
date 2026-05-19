#ifndef CONNECT_HPP
#define CONNECT_HPP

#include <iostream>
#include <thread>
#include "locdata.hpp"
#include "platform_socket.hpp"
#include "logger.hpp"

void connection_thread(SocketResource socketResource, bool is_master) {
    Logger logger("Connection");
    if (!socketResource.is_available()) {
        logger.log("socketResource was not initialised");
        return;
    }

    curr_state.store(GameState_InGame, std::memory_order_release);
    int alive = 1;
    if (is_master) {
        ssize_t nbytes = send(socketResource, &alive, sizeof(alive), 0);
        logger.log("Sending first heartbeat ", nbytes);
    }

    while (curr_state.load(std::memory_order_acquire) == GameState_InGame) {
        ssize_t nbytes = recv(socketResource, &alive, sizeof(alive), 0);
        if (nbytes == 0) {
            logger.log("Socket is dead. Breaking.");
            break;
        } else {
            logger.log("Recved heartbeat ", nbytes);
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
        nbytes = send(socketResource, &alive, sizeof(alive), 0);
        logger.log("Sending next heartbeat ", nbytes);
    }
    curr_state.store(GameState_Available, std::memory_order_release);
    logger.log("Closing heartbeat");
};

int invite_user(const LocData& locdata) {
    Logger logger("Invite");
    SocketResource socketResource(AF_INET, SOCK_STREAM, 0);
    if (!socketResource.is_available()) {
        logger.log("Failed to create socket ", socket_error());
        return -1;
    }

    if (socketResource.setsockopt(SO_REUSEADDR, 1)) {
        logger.log("Failed to set socket to be reusable");
        return -1;
    }

    sockaddr_in serverAddress = create_sockaddr(locdata.address, locdata.port);
    if (socketResource.connect(serverAddress)) {
        logger.log("Failed to connect to tcp server: ", socket_error());
        return -1;
    }

    std::thread _conn_thread(connection_thread, std::move(socketResource), false);
    _conn_thread.detach();
    return -1;
}

int tcp_server_thread(int tcpPort) {
    Logger logger("TCP");
    SocketResource socketResource(AF_INET, SOCK_STREAM, 0);
    if (!socketResource.is_available()) {
        logger.log("Failed to create socket ", socket_error());
        return -1;
    }
    if (socketResource.setsockopt(SO_REUSEADDR, 1)) {
        logger.log("Failed to set socket to be reusable ", socket_error());
        return -1;
    }

    sockaddr_in serverAddr = create_sockaddr(INADDR_ANY, tcpPort);
    if (socketResource.bind(serverAddr)) {
        logger.log("Failed to bind sever to port", socket_error());
        return -1;
    }
    
    if (socketResource.listen(1)) {
        logger.log("Failed to prepare serversocket to listen", socket_error());
        return -1;
    }
    logger.log("Listening for connections");

    curr_state.store(GameState_Available, std::memory_order_relaxed);
    while (is_running.load(std::memory_order_relaxed)) {
        sockaddr_in clientAddr;
        SocketResource clientResource = socketResource.accept(clientAddr);
        if (!clientResource.is_available()) {
            logger.log("Accept failed ", socket_error());
            continue;
        }
        connection_thread(std::move(clientResource), true);
    }
    return 0;
}

#endif
