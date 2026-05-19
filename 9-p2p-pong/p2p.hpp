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

#define LOOP_INTERVAL 10

void broadcast_thread(int udpSocket, int udpPort, LocData myloc) {
    std::cout << "Starting broadcast thread\n";

    sockaddr_in broadcastAddress{};
    broadcastAddress.sin_family = AF_INET;
    broadcastAddress.sin_port = htons(udpPort); // converts to network byte order
    broadcastAddress.sin_addr.s_addr = INADDR_BROADCAST; // Socket listens to all available IPs (Main TCP socket to start handshake)
    while (is_running.load(std::memory_order_relaxed)) {
        myloc.refresh(curr_state.load(std::memory_order_acquire));
        ssize_t n = sendto(udpSocket, &myloc, sizeof(myloc), 0, reinterpret_cast<sockaddr*>(&broadcastAddress), sizeof(broadcastAddress));
        std::cout << "Sending UDP n = " << n << " to broadcast port " << udpPort << ". Error: " << socket_error() <<"\n";
        std::this_thread::sleep_for(std::chrono::seconds(LOOP_INTERVAL));
    }
    std::cout << "Closed broadcast thread\n";
}

void listen_thread(int clientSocket, int udpPort) {
    std::cout << "Starting listen thread\n";
    sockaddr_in senderAddress;
    socklen_t addressSize = sizeof(senderAddress);
    LocData recvloc;
    while (is_running.load(std::memory_order_relaxed)) {
        ssize_t n = recvfrom(clientSocket, &recvloc, sizeof(recvloc), 0, reinterpret_cast<sockaddr*>(&senderAddress), &addressSize);
        std::cout << "Received n = " << n << " bytes \n";
        if (n <= 0) {
            std::cerr << "Failed to receive UDP gateway response: " << socket_error() << "\n";
            break;
        }
        recvloc.address = senderAddress.sin_addr.s_addr;
        {
            std::unique_lock lock(neighbour_ips_mut);
            neighbour_ips[recvloc.id()] =  recvloc;
            auto it = neighbour_ips.begin();
            time_t now = Clock::to_time_t(Clock::now());
            while (it != neighbour_ips.end()) {
                if (std::difftime(now, it->second.timestamp) > LOOP_INTERVAL) {
                    it = neighbour_ips.erase(it);
                } else {
                    it++;
                }
            }
        }
    }
    std::cout << "Closed listen thread\n";
}

struct SearchThreads {
    int clientSocket = -1;

    SearchThreads(int udpPort, int tcpPort, std::string_view name) {
        clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (clientSocket < 0) {
            std::cerr << "Failed to create UDP socket: " << socket_error() << "\n";
            return;
        } 

        int is_broadcast = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&is_broadcast), sizeof(is_broadcast)) != 0) {
            std::cout << "Failed to set SO_BROADCAST: " << socket_error() << "\n";
            return;
        }

        int reuse = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&reuse), sizeof(reuse)) != 0) {
            std::cout << "Failed to set SO_REUSEPORT: " << socket_error() << "\n";
            return;
        }

        sockaddr_in listenAddress{};
        listenAddress.sin_family = AF_INET;
        listenAddress.sin_port = htons(udpPort); // converts to network byte order
        listenAddress.sin_addr.s_addr = INADDR_ANY; // Socket listens to all available IPs (Main TCP socket to start handshake)
        if (bind(clientSocket, reinterpret_cast<sockaddr*>(&listenAddress), sizeof(listenAddress))) {
            std::cout << "Failed to bind listen address: " << socket_error() << "\n";
            return;
        }

        LocData locData(tcpPort, name);
        is_running.store(true, std::memory_order_relaxed);
        std::thread _broadcast_thread(broadcast_thread, clientSocket, udpPort, locData);
        _broadcast_thread.detach();

        std::thread _listen_thread(listen_thread, clientSocket, udpPort);
        _listen_thread.detach();
    }

    ~SearchThreads() {
        is_running.store(false, std::memory_order_relaxed);
        if (clientSocket >= 0) close(clientSocket);
    }
};


#endif