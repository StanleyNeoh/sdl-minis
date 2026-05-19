#ifndef CONNECT_HPP
#define CONNECT_HPP

#include <iostream>
#include <thread>
#include "globals.hpp"
#include "platform_socket.hpp"
#include "logger.hpp"
#include "p2p.hpp"

namespace Connection {
    struct Config {
        std::atomic<GameState>* currState;
        in_port_t tcpPort;
    };

    void main(SocketResource socketResource, bool is_master) {
        Logger logger("Connection");
        if (!socketResource.is_available()) {
            logger.log("socketResource was not initialised");
            return;
        }

        currState.store(GameState_InGame, std::memory_order_release);
        int alive = 1;
        if (is_master) {
            ssize_t nbytes = socketResource.send(&alive, sizeof(alive));
            logger.log("Sending first heartbeat ", nbytes);
        }

        while (currState.load(std::memory_order_acquire) == GameState_InGame) {
            ssize_t nbytes = socketResource.recv(&alive, sizeof(alive));
            if (nbytes <= 0) {
                logger.log("Socket is dead. Breaking.");
                break;
            } else {
                logger.log("Recved heartbeat ", nbytes);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            nbytes = socketResource.send(&alive, sizeof(alive));
            logger.log("Sending next heartbeat ", nbytes);
        }
        currState.store(GameState_Available, std::memory_order_release);
        logger.log("Closing heartbeat");
    };

    int connect_user(const P2P::LocData& locdata) {
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

        std::thread _conn_thread(main, std::move(socketResource), false);
        _conn_thread.detach();
        return -1;
    }

    int server(Config config) {
        Logger logger("Server");
        SocketResource socketResource(AF_INET, SOCK_STREAM, 0);
        if (!socketResource.is_available()) {
            logger.log("Failed to create socket ", socket_error());
            return -1;
        }
        if (socketResource.setsockopt(SO_REUSEADDR, 1)) {
            logger.log("Failed to set socket to be reusable ", socket_error());
            return -1;
        }

        sockaddr_in serverAddr = create_sockaddr(INADDR_ANY, config.tcpPort);
        if (socketResource.bind(serverAddr)) {
            logger.log("Failed to bind sever to port", socket_error());
            return -1;
        }
        
        if (socketResource.listen(1)) {
            logger.log("Failed to prepare serversocket to listen", socket_error());
            return -1;
        }
        logger.log("Listening for connections");

        currState.store(GameState_Available, std::memory_order_release);
        while (isRunning.load(std::memory_order_relaxed)) {
            sockaddr_in clientAddr;
            SocketResource clientResource = socketResource.accept(clientAddr);
            if (!clientResource.is_available()) {
                logger.log("Accept failed ", socket_error());
                continue;
            }
            if (currState.load(std::memory_order_acquire) == GameState_Available) {
                main(std::move(clientResource), true);
            }
        }
        return 0;
    }
}

#endif
