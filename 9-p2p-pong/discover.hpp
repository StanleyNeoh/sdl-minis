#ifndef P2P_HPP
#define P2P_HPP

#include <chrono>
#include <array>
#include <string_view>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <cstring>
#include "platform_socket.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "pipe.hpp"

struct Discover {
    constexpr static size_t MAX_NEIGH = 1024;
    constexpr static size_t MAX_NAME_SIZE = 31;
    enum State {
        IsHost,
        Available,
        Unavailable,
        Closed
    };

    struct LocPacket {
        in_port_t port = 0;
        State state = Available;
        char name[32] = {0};

        LocPacket() = default;
        LocPacket(in_port_t port, std::string_view _name): port(port) {
            _name = _name.substr(0, MAX_NAME_SIZE);
            memcpy(name, _name.data(), _name.size());
        }

        friend std::ostream& operator<<(std::ostream& o, const LocPacket& locPacket) {
            o << locPacket.name << "=?:" << ntohs(locPacket.port);
            return o;
        }
    };

    struct Loc {
        sockaddr_in address;
        State state = Available;
        char name[MAX_NAME_SIZE + 1] = {0};

        Loc() = default;
        Loc(const sockaddr_in& address, std::string_view _name, State state = Available): address(address), state(state) {
            _name = _name.substr(0, MAX_NAME_SIZE);
            memcpy(name, _name.data(), _name.size());
        }
        Loc(const LocPacket& packet, in_addr_t address): Loc(create_sockaddr(address, packet.port, true), packet.name, packet.state) {}

        bool operator==(const Loc& other) const {
            return address == other.address;
        }

        LocPacket to_packet() const {
            return LocPacket(address.sin_port, name);
        }

        size_t id() {
            return address.sin_addr.s_addr << 16 | address.sin_port;
        }

        friend std::ostream& operator<<(std::ostream& o, const Loc& loc) {
            o << loc.name << "=" << loc.address;
            return o;
        }
    };

    struct Config {
        Loc ownLoc;
        in_port_t udpPort = 12345;
        time_t loop_interval = 1;
        double awake_interval = 10;

        Config(std::string_view name, u_int16_t port): ownLoc(create_sockaddr(own_ip_address(), port), name) {}
    };

    const Config config;
    std::shared_mutex neighIpsMut;
    std::array<Loc, MAX_NEIGH> neighIps;
    size_t neighIpsSize = 0;
    SPSCQueue<Loc> incoming;
    std::atomic<bool> isRunning = true;
    std::thread _io_thread;
    std::thread _work_thread;

    void upsert_neighbour(const Loc& loc) {
        std::unique_lock _lock(neighIpsMut);
        bool found = false;
        for (int i = 0; i < neighIpsSize; i++) {
            auto& p = neighIps[i];
            if (p == loc) {
                p = loc;
                found = true;
                break;
            }
        }
        if (!found && neighIpsSize < MAX_NEIGH) {
            neighIps[neighIpsSize++] = loc;
        }
    }

    void delete_neighbour(const Loc& loc) {
        std::unique_lock _lock(neighIpsMut);
        int i = 0;
        bool found = false;
        for (; i < neighIpsSize; i++) {
            if (neighIps[i] == loc) {
                found = true;
                break;
            }
        }
        if (found) {
            neighIpsSize--;
            std::swap(neighIps[i], neighIps[neighIpsSize]);
        }
    }

    static void io_thread(Discover* obj) {
        Logger logger("Discover IO");
        logger.log("Starting discover io thread");
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
        if (socketResource.setsockopt(SO_RCVTIMEO, timeval{.tv_sec = obj->config.loop_interval, .tv_usec = 0})) {
            logger.log("Failed to set SO_RECVTIMEO: ", socket_error());
            return;
        }
        sockaddr_in listenAddress = create_sockaddr(INADDR_ANY, obj->config.udpPort);
        if (socketResource.bind(listenAddress)) {
            logger.log("Failed to bind listen address: ", socket_error());
            return;
        }

        sockaddr_in broadcastAddress = create_sockaddr(INADDR_BROADCAST, obj->config.udpPort);
        sockaddr_in senderAddress;
        LocPacket ownLocPac = obj->config.ownLoc.to_packet();
        LocPacket recvPac;
        time_t last_send = -1;
        ssize_t n = socketResource.sendto(&ownLocPac, sizeof(ownLocPac), broadcastAddress);
        logger.log("Sending initial discover UDP size = ", n, " data = ", ownLocPac);
        while (obj->isRunning.load(std::memory_order_relaxed)) {
            n = socketResource.recvfrom(&recvPac, sizeof(recvPac), senderAddress);
            if (n > 0) {
                Loc receivedLoc(recvPac, senderAddress.sin_addr.s_addr);
                logger.log("Received ", recvPac, " to ", receivedLoc);
                Backoff backoff;
                while (obj->isRunning.load(std::memory_order_relaxed) && !obj->incoming.push(receivedLoc)) {
                    backoff.backoff();
                }
            }
            time_t now = curr_time();
            if (std::difftime(now, last_send) > obj->config.awake_interval) {
                logger.log("Resending discover UDP size data = ", ownLocPac);
                n = socketResource.sendto(&ownLocPac, sizeof(ownLocPac), broadcastAddress);
                last_send = now;
            }
        }
        ownLocPac.state = Closed;
        n = socketResource.sendto(&ownLocPac, sizeof(ownLocPac), broadcastAddress);
        logger.log("Sending discover closing UDP size = ", n, " to broadcast port ", obj->config.udpPort, ". Error: ", socket_error());
        logger.log("Closed dicover io thread");
    }

    static void work_thread(Discover* obj) {
        Logger logger("Discover work");
        logger.log("Starting discover work thread");
        Loc loc;
        Backoff backoff;
        while (obj->isRunning.load(std::memory_order_relaxed)) {
            while (obj->incoming.pop(loc)) {
                backoff.reset();
                switch (loc.state) {
                    case Closed:
                        obj->delete_neighbour(loc);
                        break;
                    default:
                        obj->upsert_neighbour(loc);
                        break;
                }
            }
            backoff.backoff();
        }
        logger.log("Closed dicover work thread");
    }

    Discover(const Config& config): 
        config(config),
        _io_thread(io_thread, this),
        _work_thread(work_thread, this) {}
    
    void kill() {
        isRunning.store(false, std::memory_order_relaxed);
        _io_thread.join();
        _work_thread.join();
    }

    std::vector<Loc> getNeighbours() {
        std::vector<Loc> ret;
        ret.reserve(MAX_NEIGH);
        std::shared_lock lock(neighIpsMut);
        for (int i = 0; i < neighIpsSize; i++) {
            ret.push_back(neighIps[i]);
            if (neighIps[i] == config.ownLoc) {
                ret.back().state = IsHost;
            }
        }
        return ret;
    }
};

#endif
