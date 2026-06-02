#ifndef P2PTCP_HPP
#define P2PTCP_HPP

#include <thread>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <SDL.h>
#include "platform_socket.hpp"
#include "pipe.hpp"
#include "logger.hpp"
#include "pong.hpp"
#include "packet.hpp"

struct TcpManager {
    struct Config {
        u_int16_t port;

        Config(u_int16_t port): port(port) {}
    };

    const Config config;
    std::thread manager_thread;
    SPSCQueue<Packet::Packet> incomingQueue;
    SPSCQueue<Packet::Packet> outgoingQueue;

    // Used by manager thread only
    bool running; 

    static void io_session(TcpManager* ctx, SocketResource socketResource) {
        Logger logger("TCP IO");
        if (!socketResource.set_blocking<false>()) {
            logger.log("Failed to set non blocking");
            return;
        }
        bool disconnected = false;
        Packet::Reader reader;
        Packet::Writer writer;
        Packet::Packet packet;
        while (!disconnected) {
            auto status = reader.recv(socketResource, packet);
            switch (status) {
            case Packet::Reader::Recved:
                ctx->incomingQueue.push(std::move(packet));
                break;
            case Packet::Reader::Closed:
                disconnected = true;
                break;
            case Packet::Reader::Invalid:
                logger.log("Received invalid wire packet");
                disconnected = true;
                break;
            default:
                break;
            }

            while (ctx->outgoingQueue.try_pop(packet)) {
                // Use compile-time trait checking
                if (packet.type == Packet::KillRequestType) {
                    ctx->running = false;
                }
                
                if (Packet::is_disconnect(packet.type)) {
                    disconnected = true;
                }
                
                // Send wireable packets
                if (Packet::is_wireable(packet.type)) {
                    writer.send(socketResource, packet);
                }
            }
        }
    }

    static void manager(TcpManager* ctx) {
        Logger logger("TCP Manager");
        SocketResource listener(AF_INET, SOCK_STREAM, 0);
        if (!listener.is_available()) {
            logger.log("Failed to open TCP socket");
            return;
        }

        sockaddr_in listenAddr = create_sockaddr(INADDR_ANY, ctx->config.port);
        if (listener.setsockopt(SO_REUSEADDR, 1)) {
            logger.log("Failed to set REUSEADDR");
            return;
        }
        if (listener.setsockopt(SO_REUSEPORT, 1)) {
            logger.log("Failed to set REUSEPORT");
            return;
        }
        if (!listener.set_blocking<false>()) {
            logger.log("Failed to set non blocking");
            return;
        }
        if (listener.bind(listenAddr)) {
            logger.log("Failed to bind to listen address: ", listenAddr);
            return;
        }
        if (listener.listen(1)) {
            logger.log("Failed to prepare socket to listen");
            return;
        }

        ctx->running = true;
        while (ctx->running) {
            {
                sockaddr_in clientAddr{};
                SocketResource accepted = listener.accept(clientAddr);
                if (accepted.is_available()) {
                    ctx->sendConnectResponse(clientAddr, true);
                    io_session(ctx, std::move(accepted));
                    ctx->sendDisconnectResponse(clientAddr);
                    continue;
                }
            }

            Packet::Packet packet;
            if (ctx->outgoingQueue.try_pop(packet)) {
                switch (packet.type) {
                    case Packet::ConnectRequestType: {
                        SocketResource outbound(AF_INET, SOCK_STREAM, 0);
                        if (!outbound.is_available()) {
                            logger.log("Failed to create outbound socket");
                            break;
                        }
                        if (outbound.connect(packet.data.connect_request.addr)) {
                            logger.log("Failed to connect to ", packet.data.connect_request.addr);
                            break;
                        }
                        ctx->sendConnectResponse(packet.data.connect_request.addr, false);
                        io_session(ctx, std::move(outbound));
                        ctx->sendDisconnectResponse(packet.data.connect_request.addr);
                        break;
                    }
                    case Packet::KillRequestType:
                        ctx->running = false;
                        break;
                    default:
                        break;
                }
            }
            std::this_thread::yield();
        }
    }

    TcpManager(const Config config):
        config(config),
        manager_thread(manager, this) {}

    ~TcpManager() {
        Packet::Packet packet{ .type = Packet::KillRequestType };
        outgoingQueue.push(packet);
        manager_thread.join();
    }

    bool connect(const sockaddr_in& addr) {
        return outgoingQueue.try_push(Packet::Packet{
            .type = Packet::ConnectRequestType,
            .data = { .connect_request = {
                .addr = addr
            }}
        });
    }

    void sendConnectResponse(const sockaddr_in& addr, bool is_master) {
        return incomingQueue.push(Packet::Packet{
            .type = Packet::ConnectResponseType,
            .data = { .connect_response = {
                .addr = addr,
                .is_master = is_master
            }}
        });
    }

    bool disconnect() {
        return outgoingQueue.try_push(Packet::Packet{
            .type = Packet::DisconnectRequestType,
        });
    }

    void sendDisconnectResponse(const sockaddr_in& addr) {
        incomingQueue.push(Packet::Packet{
            .type = Packet::DisconnectResponseType,
            .data = { .disconnect_response = {
                .addr = addr,
            }}
        });
    }

    bool sendMessage(const char* msg) {
        Packet::Packet packet{ .type = Packet::MessageType };
        std::memset(packet.data.message.message, 0, sizeof(packet.data.message.message));
        if (msg) {
            std::strncpy(packet.data.message.message, msg, sizeof(packet.data.message.message) - 1);
        }
        return outgoingQueue.try_push(packet);
    }

};

#endif
