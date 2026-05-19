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
#include "logger.hpp"

namespace P2P {
    using Clock = std::chrono::system_clock;
    struct LocData {
        in_addr_t address = 0;
        in_port_t port = 0;
        char name[32] = {0};
        time_t timestamp = 0;
        GameState state = GameState_Uninitialised;

        LocData(int port, std::string_view _name, in_addr_t address): port(port), address(address) {
            _name = _name.substr(0, 31);
            memcpy(name, _name.data(), _name.size());
        }
        LocData(int port, std::string_view _name): LocData(port, _name, 0) {}
        LocData(): LocData(0, "", 0) {};

        void refresh(GameState _state = GameState_Uninitialised) {
            timestamp = Clock::to_time_t(Clock::now());
            state = _state;
        }

        bool operator==(const LocData& other) const {
            return address == other.address && port == other.port;
        }

        std::size_t id() const {
            return address << 16 | port;
        }
    };

    std::shared_mutex neighbour_ips_mut;
    std::unordered_map<size_t, LocData> neighbour_ips;

    void p2p_thread(
        std::atomic<bool>* is_running,
        int udpPort, 
        int tcpPort, 
        std::string_view name, 
        int loop_interval
    ) {
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
        while (is_running->load(std::memory_order_relaxed)) {
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

    void start_p2p_thread(
        std::atomic<bool>& is_running, 
        int udpPort, 
        int tcpPort, 
        std::string_view name, 
        int loop_interval
    ) {
        std::thread _thread(p2p_thread, &is_running, udpPort, tcpPort, name, loop_interval);
        _thread.detach();
    }
}

template <>
struct std::hash<P2P::LocData> {
    std::size_t operator()(const P2P::LocData& locData) const noexcept {
        return locData.id();
    }
};


#endif