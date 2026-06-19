#ifndef DISCOVER_DISCOVER_HPP
#define DISCOVER_DISCOVER_HPP

#include <chrono>
#include <iostream>
#include <thread>
#include <sstream>
#include <unordered_map>
#include "common/common.hpp"
#include "imgui.h"
#include "loc.hpp"

namespace Discover {
    constexpr static size_t MAX_NEIGH = 1024;
    struct Action {
        enum Type {
            Kill
        };
        Type type;
    };

    struct Config {
        Loc ownLoc;
        u_int16_t discoverPort = 12345;
        time_t loop_interval = 1;
        double heartbeat_interval = 10;
        double timeout_interval = 20;

        Config() = default;
        Config(std::string_view name, u_int16_t port): ownLoc(create_sockaddr(own_ip_address(), port), name) {}
    };

    struct Discover {
        Config config;
        std::thread _io_thread;
        SPSCQueue<Action> sendQueue;
        SPSCQueue<Loc> recvQueue;
        bool is_initialised = false;

        // IO thread only
        bool isRunning = true; // Used by io_thread only

        // Local thread only
        std::unordered_map<sockaddr_in, Loc> neighbours;
        std::vector<sockaddr_in> to_remove;

        static void io_thread(Discover* ctx) {
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
            if (!socketResource.set_blocking<false>()) {
                logger.log("Failed to set Non blocking");
                return;
            }
            sockaddr_in listenAddress = create_sockaddr(INADDR_ANY, ctx->config.discoverPort);
            if (socketResource.bind(listenAddress)) {
                logger.log("Failed to bind listen address: ", socket_error());
                return;
            }

            Action action;
            Loc ownLoc = ctx->config.ownLoc;
            Loc recvLoc;
            time_t last_send = -1;
            sockaddr_in broadcastAddress = create_sockaddr(INADDR_BROADCAST, ctx->config.discoverPort);
            sockaddr_in senderAddress;

            ssize_t n = socketResource.sendto(&ownLoc, sizeof(ownLoc), broadcastAddress);
            logger.log("Sending initial discover UDP size = ", n, " data = ", ownLoc);
            while (ctx->isRunning) {
                if (ctx->sendQueue.try_pop(action)) {
                    switch (action.type) {
                        case Action::Kill:
                            ctx->isRunning = false;
                            break;
                        default:
                            break;
                    }
                }

                if (socketResource.recvfrom(&recvLoc, sizeof(recvLoc), senderAddress) > 0) {
                    recvLoc.address.sin_addr.s_addr = senderAddress.sin_addr.s_addr;
                    recvLoc.timestamp = curr_time();
                    logger.log("Received ", recvLoc);
                    ctx->recvQueue.push(recvLoc);
                };

                time_t now = curr_time();
                if (std::difftime(now, last_send) > ctx->config.heartbeat_interval) {
                    logger.log("Resending discover UDP size data = ", ownLoc);
                    n = socketResource.sendto(&ownLoc, sizeof(ownLoc), broadcastAddress);
                    last_send = now;
                }
            }

            ownLoc.state = Loc::Closed;
            n = socketResource.sendto(&ownLoc, sizeof(ownLoc), broadcastAddress);
            logger.log("Sending discover closing UDP size = ", n, " to broadcast port ", ctx->config.discoverPort, ". Error: ", socket_error());
            logger.log("Closed discover io thread");
        }

        bool initialise(const Config& _config) {
            if (is_initialised) return false;
            is_initialised = true;
            config = _config;
            _io_thread = std::thread(io_thread, this);
            return true;
        }
        
        ~Discover() {
            if (!is_initialised) return;
            sendQueue.push(Action{.type=Action::Kill});
            _io_thread.join();
        }

        void process_events() {
            Loc loc;
            while (recvQueue.try_pop(loc)) {
                neighbours[loc.address] = loc;
            }

            to_remove.clear();
            time_t now = curr_time();
            for (auto& p: neighbours) {
                if (std::difftime(now, p.second.timestamp) > config.timeout_interval) {
                    to_remove.push_back(p.first);
                }
            }

            for (auto& p: to_remove) {
                neighbours.erase(p);
            }
        }

        void draw();
    };

    extern Discover discover;
}

#endif
