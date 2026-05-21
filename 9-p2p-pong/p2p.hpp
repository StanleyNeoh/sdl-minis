#ifndef P2P_HPP
#define P2P_HPP

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <cstring>
#include <unordered_map>
#include <memory>
#include "platform_socket.hpp"
#include "globals.hpp"
#include "logger.hpp"
#include "utils.hpp"

namespace P2P {
    struct LocPacket {
        in_port_t port = 0;
        AppState state = AppState_Available;
        char name[32] = {0};

        LocPacket() = default;
        LocPacket(int port, std::string_view _name): port(port) {
            _name = _name.substr(0, 31);
            memcpy(name, _name.data(), _name.size());
        }
    };

    struct LocData: LocPacket {
        in_addr_t address = 0;

        LocData() = default;
        LocData(in_addr_t address, in_port_t port, std::string_view name): LocPacket(port, name), address(address) {}
        LocData(LocPacket& packet, in_addr_t address): 
            LocPacket(packet), 
            address(address) {}

        bool operator==(const LocData& other) const {
            return address == other.address && port == other.port;
        }

        std::size_t id() const {
            return address << 16 | port;
        }
    };

    inline std::ostream& operator<<(std::ostream& o, const P2P::LocData& locData) {
        char clientIp[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &locData.address, clientIp, sizeof(clientIp));
        o << clientIp << ":" << locData.port;
        return o;
    }

    struct Config {
        std::atomic<AppState>* currState;
        std::string_view name;
        in_port_t gamePort;
        in_port_t udpPort = 12345;
        int loop_interval = 1;
    };

    std::shared_mutex neighbour_ips_mut;
    std::vector<std::unique_ptr<LocData>> neighbour_ips;

    void upsert_neighbour(const LocData& recvloc) {
        std::unique_lock _lock(neighbour_ips_mut);
        bool found = false;
        for (auto& p: neighbour_ips) {
            if (*p.get() == recvloc) {
                *p.get() = recvloc;
                found = true;
                break;
            }
        }
        if (!found) {
            neighbour_ips.push_back(std::make_unique<LocData>(recvloc));
        }
    }

    void delete_neighbour(const LocData& recvloc) {
        std::unique_lock _lock(neighbour_ips_mut);
        int n = neighbour_ips.size();
        int i = 0;
        for (; i < n; i++) {
            if (*neighbour_ips[i] == recvloc) break;
        }
        if (i != n) {
            std::swap(neighbour_ips[i], neighbour_ips.back());
            neighbour_ips.pop_back();
        }
    }

    void main(Config config) {
        Logger<false> logger("P2P");
        logger.log("Starting p2p thread");
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
        if (socketResource.setsockopt(SO_RCVTIMEO, timeval{.tv_sec = config.loop_interval, .tv_usec = 0})) {
            logger.log("Failed to set SO_RECVTIMEO: ", socket_error());
            return;
        }
        sockaddr_in listenAddress = create_sockaddr(INADDR_ANY, config.udpPort);
        if (socketResource.bind(listenAddress)) {
            logger.log("Failed to bind listen address: ", socket_error());
            return;
        }

        LocPacket myloc(config.gamePort, config.name);
        sockaddr_in broadcastAddress = create_sockaddr(INADDR_BROADCAST, config.udpPort);
        sockaddr_in senderAddress;
        socklen_t addressSize = sizeof(senderAddress);
        while (config.currState->load(std::memory_order_relaxed) != AppState_Closed) {
            myloc.state = config.currState->load(std::memory_order_relaxed);
            ssize_t n = socketResource.sendto(&myloc, sizeof(myloc), broadcastAddress);
            logger.log("Sending UDP n = ", n, " to broadcast port ", config.udpPort, ". Error: ", socket_error());

            while (true) {
                LocPacket recvPac;
                n = socketResource.recvfrom(&recvPac, sizeof(recvPac), senderAddress);
                logger.log("Received UDP n = ", n, " to broadcast port ", config.udpPort, ". Error: ", socket_error());
                if (n < 0) break;

                LocData recvloc(recvPac, senderAddress.sin_addr.s_addr);
                switch (recvloc.state) {
                    case AppState_Closed:
                        delete_neighbour(recvloc);
                        break;
                    default:
                        upsert_neighbour(recvloc);
                        break;
                }
            }
        }

        myloc.state = AppState_Closed;
        ssize_t n = socketResource.sendto(&myloc, sizeof(myloc), broadcastAddress);
        logger.log("Closed p2p thread");
    }
}

template <>
struct std::hash<P2P::LocData> {
    std::size_t operator()(const P2P::LocData& locData) const noexcept {
        return locData.id();
    }
};

#endif
