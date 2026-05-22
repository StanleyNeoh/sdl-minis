#ifndef CONNECT_HPP
#define CONNECT_HPP

#include <iostream>
#include <thread>
#include "globals.hpp"
#include "platform_socket.hpp"
#include "logger.hpp"
#include "discover.hpp"
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
    constexpr double TIMEOUT_DURATION_S = 10;
    constexpr time_t TIMEOUT_CHECK_S = 1;
    struct Config {
        std::atomic<AppState>* currState;
        in_port_t gamePort;
    };

    struct Packet {
        enum Status {
            UNINITIALISED,
            INVITATION_NEW,
            INVITATION_ACCEPTED,
            HEARTBEAT,
            EVENT,
        };

        Status status = UNINITIALISED;

        friend std::ostream& operator<<(std::ostream& o, const Packet& p) {
            switch(p.status) {
                case UNINITIALISED:
                    o << "UNINITIALISED";
                    break;
                case INVITATION_NEW:
                    o << "INVITATION_NEW";
                    break;
                case INVITATION_ACCEPTED:
                    o << "INVITATION_ACCEPTED";
                    break;
                case HEARTBEAT:
                    o << "HEARTBEAT";
                    break;
                case EVENT:
                    o << "EVENT";
                    break;
                default:
                    o << "?";
                    break;
            }
            return o;
        }
    };


    void connection_loop(SocketResource& socketResource, const sockaddr_in& oppAddress) {
        Logger logger("Connection Loop");
        logger.log("Begin connection loop");
        // 2. Begin game
        Packet heartBeat{.status = Packet::HEARTBEAT};
        while (true) {
            Packet packet; sockaddr_in senderAddress;
            ssize_t n = socketResource.recvfrom(&packet, sizeof(packet), senderAddress);
            if (n < 0) {
                logger.log("Sending heartbeat");
                socketResource.sendto(&heartBeat, sizeof(heartBeat), oppAddress);
                continue;
            }
            if (senderAddress != oppAddress) continue;
            logger.log("Received heartbeat");
        }
        logger.log("Closing connection loop");
    }


    void client(SocketResource socketResource, sockaddr_in oppAddress) {
        Logger logger("Client");
        logger.log("Begin client with oppAddress: ", oppAddress);
        time_t last_time = curr_time();
        // 1. Wait for accept from server
        while (true) {
            Packet packet; sockaddr_in senderAddress;
            ssize_t n = socketResource.recvfrom(&packet, sizeof(packet), senderAddress);
            time_t now = curr_time();
            if (std::difftime(now, last_time) > TIMEOUT_DURATION_S) {
                logger.log("Timed out");
                return;
            }
            if (n < 0) continue;
            logger.log("Received packet from ", senderAddress, " with data: ", packet);
            if (packet.status == Packet::INVITATION_ACCEPTED && senderAddress == oppAddress) {
                logger.log("Received invite acceptance from", oppAddress);
                n = socketResource.sendto(&packet, sizeof(packet), oppAddress);
                if (n < 0) {
                    logger.log("Failed to send acknowledgement to ", oppAddress);
                } else {
                    logger.log("Successfully sent acknowledgement to ", oppAddress);
                }
                break;
            }
        }
        logger.log("Transferring to connection loop");

        // Begin game
        connection_loop(socketResource, oppAddress);
    }

    void connect_user(const Config& config, const Discover::Loc& locdata) {
        Logger logger("Connect User");
        SocketResource socketResource(AF_INET, SOCK_DGRAM, 0);
        if (!socketResource.is_available()) {
            logger.log("Failed to create socket ", socket_error());
            return;
        }
        if (socketResource.setsockopt(SO_REUSEADDR, 1)) {
            logger.log("Failed to set socket to be reusable");
            return;
        }
        if (socketResource.setsockopt(SO_RCVTIMEO, timeval{.tv_sec=TIMEOUT_CHECK_S, .tv_usec=0})) {
            logger.log("Failed to set socket to be reusable");
            return;
        }
        sockaddr_in listenAddress = create_sockaddr(INADDR_ANY, config.gamePort);
        if (socketResource.bind(listenAddress)) {
            logger.log("Failed to bind to listen address");
            return;
        }
        Packet packet{.status = Packet::INVITATION_NEW};
        const sockaddr_in& serverAddress = locdata.address;
        ssize_t n = socketResource.sendto(&packet, sizeof(packet), serverAddress);
        if (n < 0) {
            logger.log("Failed to send invite to ", serverAddress);
            return;
        } else {
            logger.log("Successfully sent invite to ", serverAddress);
        }
        std::thread client_thread(client, std::move(socketResource), std::move(serverAddress));
        client_thread.detach();
        return;
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
        while (config.currState->load(std::memory_order_relaxed) != AppState_Closed) {
            Packet packet; sockaddr_in clientAddr;
            ssize_t n = socketResource.recvfrom(&packet, sizeof(packet), clientAddr);
            time_t now = curr_time();
            if (pending_accept && std::difftime(now, last_time) > TIMEOUT_DURATION_S) {
                logger.log("Timed out");
                pending_accept = false;
            }
            if (n <= 0) continue;
            logger.log("Received packet from ", clientAddr, " with data: ", packet);

            switch (packet.status) {
            case Packet::INVITATION_NEW:
                if (pending_accept) break;
                logger.log("Received new invite from ", clientAddr);
                pending_accept = true;
                last_time = now;
                oppAddress = clientAddr;
                packet.status = Packet::INVITATION_ACCEPTED;
                n = socketResource.sendto(&packet, sizeof(packet), clientAddr);
                if (n < 0) {
                    logger.log("Failed to send invite accept to ", clientAddr);
                } else {
                    logger.log("Successfully send invite accept to ", clientAddr);
                }
                break;
            case Packet::INVITATION_ACCEPTED: 
                // Game begin
                if (oppAddress != clientAddr) break;
                logger.log("Handshake completed with ", oppAddress);
                connection_loop(socketResource, oppAddress);
                break;
            default:
                break;
            }
        }
        logger.log("Closing server thread");
        return 0;
    }
}

#endif
