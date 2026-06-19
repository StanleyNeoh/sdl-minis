#ifndef P2P_MANAGER_HPP
#define P2P_MANAGER_HPP

#include <thread>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <SDL.h>
#include "common/platform_socket.hpp"
#include "common/pipe.hpp"
#include "common/logger.hpp"
#include "packet.hpp"
#include "reader.hpp"
#include "type.hpp"

namespace P2P {
    struct TcpManager {
        struct Config {
            u_int16_t port;
            Config() = default;
            Config(u_int16_t port): port(port) {}
        };

        Config config;
        std::thread listening_thread;
        SPSCQueue<Packet> incomingQueue;
        SPSCQueue<Packet> outgoingQueue;
        bool is_initialised = false;

        // Used by listener routine only
        bool running; 

        static void session_routine(TcpManager* ctx, SocketResource socket_resource) {
            Logger logger("TCP IO");
            if (!socket_resource.set_blocking<false>()) {
                logger.log("Failed to set non blocking");
                return;
            }
            bool disconnected = false;
            Reader reader;
            Packet packet;
            while (!disconnected) {
                auto status = reader.recv(socket_resource, packet);
                switch (status) {
                case Reader::Recved:
                    ctx->incomingQueue.push(std::move(packet));
                    break;
                case Reader::Closed:
                    disconnected = true;
                    break;
                case Reader::Invalid:
                    logger.log("Received invalid wire packet");
                    disconnected = true;
                    break;
                default:
                    break;
                }

                while (ctx->outgoingQueue.try_pop(packet)) {
                    if (packet.type == KillRequestType) {
                        ctx->running = false;
                    }

                    if (packet.is_disconnect()) {
                        disconnected = true;
                    }
                    
                    // Send wireable packets
                    if (packet.is_wireable()) {
                        packet.send(socket_resource);
                    }
                }
            }
        }

        static void listener_routine(TcpManager* ctx) {
            Logger logger("TCP Manager");
            SocketResource listening_socket(AF_INET, SOCK_STREAM, 0);
            if (!listening_socket.is_available()) {
                logger.log("Failed to open TCP socket");
                return;
            }

            sockaddr_in listen_addr = create_sockaddr(INADDR_ANY, ctx->config.port);
            if (listening_socket.setsockopt(SO_REUSEADDR, 1)) {
                logger.log("Failed to set REUSEADDR");
                return;
            }
            if (listening_socket.setsockopt(SO_REUSEPORT, 1)) {
                logger.log("Failed to set REUSEPORT");
                return;
            }
            if (!listening_socket.set_blocking<false>()) {
                logger.log("Failed to set non blocking");
                return;
            }
            if (listening_socket.bind(listen_addr)) {
                logger.log("Failed to bind to listen address: ", listen_addr);
                return;
            }
            if (listening_socket.listen(1)) {
                logger.log("Failed to prepare socket to listen");
                return;
            }

            ctx->running = true;
            while (ctx->running) {
                {
                    sockaddr_in clientAddr{};
                    SocketResource accepted = listening_socket.accept(clientAddr);
                    if (accepted.is_available()) {
                        ctx->sendConnectResponse(clientAddr, true);
                        session_routine(ctx, std::move(accepted));
                        ctx->sendDisconnectResponse(clientAddr);
                        continue;
                    }
                }

                Packet packet;
                if (ctx->outgoingQueue.try_pop(packet)) {
                    switch (packet.type) {
                        case ConnectRequestType: {
                            auto&& connect_request = packet.body.get<ConnectRequestBody>();
                            SocketResource outbound(AF_INET, SOCK_STREAM, 0);
                            if (!outbound.is_available()) {
                                logger.log("Failed to create outbound socket");
                                break;
                            }
                            if (outbound.connect(connect_request.addr)) {
                                logger.log("Failed to connect to ", connect_request.addr);
                                break;
                            }
                            ctx->sendConnectResponse(connect_request.addr, false);
                            session_routine(ctx, std::move(outbound));
                            ctx->sendDisconnectResponse(connect_request.addr);
                            break;
                        }
                        case KillRequestType:
                            ctx->running = false;
                            break;
                        default:
                            break;
                    }
                }
                std::this_thread::yield();
            }
        }


        bool initialise(const Config& _config) {
            if (is_initialised) return false;
            is_initialised = true;
            config = _config;
            listening_thread = std::thread(listener_routine, this);
            return true;
        }

        ~TcpManager() {
            if (!is_initialised) return;
            outgoingQueue.push(Packet::create(
                KillRequestBody{}
            ));
            listening_thread.join();
        }

        bool connect(const sockaddr_in& addr) {
            return outgoingQueue.try_push(Packet::create(
                ConnectRequestBody{
                    .addr = addr
                }
            ));
        }

        void sendConnectResponse(const sockaddr_in& addr, bool is_master) {
            RoleType role_type = is_master ? RoleType_Master : RoleType_Client;
            return incomingQueue.push(Packet::create(
                ConnectResponseBody{
                    .addr = addr,
                    .role_type = role_type
                }
            ));
        }

        bool disconnect() {
            return outgoingQueue.try_push(Packet::create(
                DisconnectRequestBody{}
            ));
        }

        void sendDisconnectResponse(const sockaddr_in& addr) {
            incomingQueue.push(Packet::create(
                DisconnectResponseBody{
                    .addr = addr,
                }
            ));
        }

        bool sendMessage(const char* msg) {
            MessageBody message;
            std::memset(message.message, 0, sizeof(message.message));
            if (msg) {
                std::strncpy(message.message, msg, sizeof(message.message) - 1);
            }
            return outgoingQueue.try_push(Packet::create(message));
        }
    };

    extern TcpManager tcp_manager;
}


#endif
