#ifndef CONNECT_HPP
#define CONNECT_HPP

#include <iostream>
#include <thread>
#include "globals.hpp"
#include "platform_socket.hpp"
#include "logger.hpp"
#include "p2p.hpp"
#include <SDL.h>
#include <atomic>
#include <chrono>

// UDP to establish game connection
// Every application maintains a server thread which listens for udp packets on gamePort.
//  - gamePort and host_ip was published on p2p module for other hosts on same network
// To start a game with another host, send a game invite to gamePort of the other host
//  - Create a new thread to listen to replies of invitation
// Other host will send back whether it accepts
// If accepts game proceeds

namespace Connection {
    struct Config {
        std::atomic<AppState>* currState;
        in_port_t gamePort;
    };

    enum PacketStatus {
        PACKET_UNINITIALISED,
        PACKET_INVITATION_NEW,
        PACKET_INVITATION_ACCEPTED,
        PACKET_HEARTBEAT,
        PACKET_EVENT,
    };

    struct Packet {
        PacketStatus status = PACKET_UNINITIALISED;
    };

    constexpr double TIMEOUT_DURATION_S = 10;
    constexpr time_t TIMEOUT_CHECK_S = 1;

    void connection_loop(SocketResource& socketResource, const sockaddr_in& oppAddress) {
        Logger logger("Connection Loop");
        // 2. Begin game
        Packet heartBeat{.status = PACKET_HEARTBEAT};
        while (true) {
            Packet packet; sockaddr_in senderAddress;
            ssize_t n = socketResource.recvfrom(&packet, sizeof(packet), senderAddress);
            if (n < 0) {
                socketResource.sendto(&heartBeat, sizeof(heartBeat), oppAddress);
                continue;
            }
            if (memcmp(&senderAddress, &oppAddress, sizeof(senderAddress)) != 0) continue;
            logger.log("Received packet");
        }
    }


    void client(SocketResource socketResource, sockaddr_in oppAddress) {
        Logger logger("Client");
        time_t last_time = curr_time();
        // 1. Wait for accept from server
        while (true) {
            Packet packet; sockaddr_in senderAddress;
            ssize_t n = socketResource.recvfrom(&packet, sizeof(packet), senderAddress);
            time_t now = curr_time();
            if (std::difftime(now, last_time) > TIMEOUT_DURATION_S) return;
            if (n < 0) continue;
            if (packet.status == PACKET_INVITATION_ACCEPTED && (memcmp(&senderAddress, &oppAddress, sizeof(senderAddress)) == 0)) break;
        }

        // Begin game
        connection_loop(socketResource, oppAddress);
    }

    int connect_user(const Config& config, const P2P::LocData& locdata) {
        Logger logger("Connect User");
        SocketResource socketResource(AF_INET, SOCK_DGRAM, 0);
        if (!socketResource.is_available()) {
            logger.log("Failed to create socket ", socket_error());
            return -1;
        }
        if (socketResource.setsockopt(SO_REUSEADDR, 1)) {
            logger.log("Failed to set socket to be reusable");
            return -1;
        }
        if (socketResource.setsockopt(SO_RCVTIMEO, timeval{.tv_sec=TIMEOUT_CHECK_S, .tv_usec=0})) {
            logger.log("Failed to set socket to be reusable");
            return -1;
        }
        sockaddr_in listenAddress = create_sockaddr(INADDR_ANY, config.gamePort);
        if (socketResource.bind(listenAddress)) {
            logger.log("Failed to bind to listen address");
            return -1;
        }
        Packet packet{.status = PACKET_INVITATION_NEW};
        sockaddr_in serverAddress = create_sockaddr(locdata.address, locdata.port);
        if (socketResource.sendto(&packet, sizeof(packet), serverAddress)) {
            logger.log("Failed to connect to tcp server: ", socket_error());
            return -1;
        }
        std::thread client_thread(client, std::move(socketResource), std::move(serverAddress));
        client_thread.detach();
        return -1;
    }

    int server(Config config) {
        Logger logger("Server");
        SocketResource socketResource(AF_INET, SOCK_DGRAM, 0);
        if (!socketResource.is_available()) {
            logger.log("Failed to create socket ", socket_error());
            return -1;
        }
        if (socketResource.setsockopt(SO_REUSEADDR, 1)) {
            logger.log("Failed to set socket to be reusable ", socket_error());
            return -1;
        }
        if (socketResource.setsockopt(SO_RCVTIMEO, timeval{.tv_sec=TIMEOUT_CHECK_S, .tv_usec=0})) {
            logger.log("Failed to set socket to be reusable ", socket_error());
            return -1;
        }
        sockaddr_in serverAddr = create_sockaddr(INADDR_ANY, config.gamePort);
        if (socketResource.bind(serverAddr)) {
            logger.log("Failed to bind sever to port", socket_error());
            return -1;
        }
        config.currState->store(AppState_Available, std::memory_order_release);
        logger.log("Listening for connections");

        time_t last_time = curr_time();
        bool pending_accept = false;
        sockaddr_in oppAddress;
        while (config.currState->load(std::memory_order_relaxed)) {
            Packet packet; sockaddr_in clientAddr;
            ssize_t n = socketResource.recvfrom(&packet, sizeof(packet), clientAddr);
            time_t now = curr_time();
            if (pending_accept && std::difftime(now, last_time) > TIMEOUT_DURATION_S) {
                pending_accept = false;
            }
            if (n <= 0) continue;

            switch (packet.status) {
            case PACKET_INVITATION_NEW:
                if (pending_accept) break;
                pending_accept = true;
                last_time = now;
                oppAddress = clientAddr;
                packet.status = PACKET_INVITATION_ACCEPTED;
                n = socketResource.sendto(&packet, sizeof(packet), clientAddr);
                break;
            case PACKET_INVITATION_ACCEPTED: 
                // Game begin
                connection_loop(socketResource, oppAddress);
                break;
            default:
                break;
            }
        }
        return 0;
    }
}

#endif
