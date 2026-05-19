#ifndef P2P_HPP
#define P2P_HPP

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <cstring>
#include <unordered_map>
#include "platform_socket.hpp"
#include "globals.hpp"
#include "locdata.hpp"
#include "logger.hpp"

void p2p_thread(int udpPort, int tcpPort, std::string_view name, int loop_interval) {
    Logger logger("P2P");
    logger.log("Starting p2p thread");
    LocData myloc(tcpPort, name);
    SocketResource socketResource(AF_INET, SOCK_DGRAM, 0);
    if (!socketResource.is_available()) {
        logger.log("Failed to create UDP socket: ", socket_error());
        return;
    } 
    if (socketResource.setsockopt(SO_BROADCAST, 1)) {
        logger.log("Failed to set SO_BROADCAST: ", socket_error());
        return;
    }
    if (socketResource.setsockopt(SO_REUSEPORT, 1)) {
        logger.log("Failed to set SO_REUSEPORT: ", socket_error());
        return;
    }
    if (socketResource.setsockopt(SO_RCVTIMEO, timeval{.tv_sec = 1, .tv_usec = 0})) {
        logger.log("Failed to set SO_RECVTIMEO: ", socket_error());
        return;
    }
    sockaddr_in listenAddress = create_sockaddr(INADDR_ANY, udpPort);
    if (socketResource.bind(listenAddress)) {
        logger.log("Failed to bind listen address: ", socket_error());
        return;
    }

    LocData recvloc;
    sockaddr_in broadcastAddress = create_sockaddr(INADDR_BROADCAST, udpPort);
    sockaddr_in senderAddress;
    socklen_t addressSize = sizeof(senderAddress);
    while (is_running.load(std::memory_order_relaxed)) {
        myloc.refresh(curr_state.load(std::memory_order_acquire));
        ssize_t n = sendto(socketResource, &myloc, sizeof(myloc), 0, sockaddr_cast(&broadcastAddress), sizeof(broadcastAddress));
        logger.log("Sending UDP n = ", n, " to broadcast port ", udpPort, ". Error: ", socket_error());
        n = recvfrom(socketResource, &recvloc, sizeof(recvloc), 0, sockaddr_cast(&senderAddress), &addressSize);
        logger.log("Received UDP n = ", n, " to broadcast port ", udpPort, ". Error: ", socket_error());
        if (n >= 0) {
            recvloc.address = senderAddress.sin_addr.s_addr;
            std::unique_lock lock(neighbour_ips_mut);
            neighbour_ips[recvloc.id()] = recvloc;
            auto it = neighbour_ips.begin();
            time_t now = Clock::to_time_t(Clock::now());
            while (it != neighbour_ips.end()) {
                if (std::difftime(now, it->second.timestamp) > loop_interval) {
                    it = neighbour_ips.erase(it);
                } else {
                    it++;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(loop_interval));
    }
    logger.log("Closed p2p thread");
}

#endif