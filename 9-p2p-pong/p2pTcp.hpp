#ifndef P2PTCP_HPP
#define P2PTCP_HPP

#include <thread>
#include <atomic>
#include <cstring>
#include <SDL.h>
#include "platform_socket.hpp"
#include "pipe.hpp"
#include "logger.hpp"
struct TcpManager {
    struct Event {
        enum Type {
            ConnectAction,
            ConnectEvent,
            DisconnectAction,
            DisconnectEvent,
            KillAction,
            Message,
            SDLEvent,
        };

        Type type;
        bool fromPeer = false;
        union {
            struct {
                sockaddr_in addr;
            } connectAction;
            struct {
                sockaddr_in addr;
                bool success;
                bool isMaster;
            } connectEvent;
            struct {
                sockaddr_in addr;
                bool success;
            } disconnectEvent;
            struct {
                SDL_Event event;
            } sdlEvent;
            struct {
                char message[128] = {0};
            } message;
        } data{};
    };

    struct EventReader {
        enum Status {
            Closed,
            Recved,
            NotRecved
        };

        ssize_t received = 0;
        char buffer[sizeof(Event)];

        Status recv(const SocketResource& socketResource, Event& event) {
            ssize_t newReceived = socketResource.recv(buffer + received, sizeof(Event) - received);
            if (newReceived == 0) return Closed;
            if (newReceived < 0) return NotRecved;
            received += newReceived;
            if (received == sizeof(Event)) {
                event = *reinterpret_cast<Event*>(buffer);
                event.fromPeer = true;
                received = 0;
                return Recved;
            }
            return NotRecved;
        }
    };

    struct Config {
        u_int16_t port;

        Config(u_int16_t port): port(port) {}
    };

    const Config config;
    std::thread manager_thread;
    SPSCQueue<Event> incomingQueue;
    SPSCQueue<Event> outgoingQueue;
    bool running; // Used by manager thread only

    static void io_session(TcpManager* ctx, SocketResource socketResource) {
        Logger logger("TCP IO");
        if (!socketResource.set_blocking<false>()) {
            logger.log("Failed to set non blocking");
            return;
        }
        bool disconnected = false;
        EventReader reader;
        Event event;
        while (!disconnected) {
            auto status = reader.recv(socketResource, event);
            switch (status) {
            case EventReader::Recved:
                ctx->incomingQueue.push(std::move(event));
                break;
            case EventReader::Closed:
                disconnected = true;
                break;
            }

            while (ctx->outgoingQueue.try_pop(event)) {
                switch (event.type) {
                case Event::ConnectAction:
                    break;
                case Event::DisconnectAction:
                    disconnected = true;
                    break;
                case Event::KillAction:
                    disconnected = true;
                    ctx->running = false;
                    break;
                default:
                    socketResource.send(&event, sizeof(event));
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
                    ctx->sendConnectEvent(clientAddr, true, true);
                    io_session(ctx, std::move(accepted));
                    ctx->sendDisconnectEvent(clientAddr);
                    continue;
                }
            }

            Event event;
            if (ctx->outgoingQueue.try_pop(event)) {
                switch (event.type) {
                    case Event::ConnectAction: {
                        SocketResource outbound(AF_INET, SOCK_STREAM, 0);
                        if (!outbound.is_available()) {
                            logger.log("Failed to create outbound socket");
                            break;
                        }
                        if (outbound.connect(event.data.connectAction.addr)) {
                            logger.log("Failed to connect to ", event.data.connectAction.addr);
                            break;
                        }
                        ctx->sendConnectEvent(event.data.connectAction.addr, true, false);
                        io_session(ctx, std::move(outbound));
                        ctx->sendDisconnectEvent(event.data.connectAction.addr);
                        break;
                    }
                    case Event::KillAction:
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
        Event event{};
        event.type = Event::KillAction;
        outgoingQueue.push(event);
        manager_thread.join();
    }

    bool connect(const sockaddr_in& addr) {
        return outgoingQueue.try_push(Event{
            .type = Event::ConnectAction,
            .data = { .connectAction = {
                .addr = addr
            }}
        });
    }

    void sendConnectEvent(const sockaddr_in& addr, bool success, bool isMaster) {
        return incomingQueue.push(Event{
            .type = Event::ConnectEvent,
            .data = { .connectEvent = {
                .addr = addr,
                .success = success,
                .isMaster = isMaster
            }}
        });
    }

    bool disconnect() {
        return outgoingQueue.try_push(Event{
            .type = Event::DisconnectAction,
        });
    }

    void sendDisconnectEvent(const sockaddr_in& addr, bool success = true) {
        incomingQueue.push(Event{
            .type = Event::DisconnectEvent,
            .data = { .disconnectEvent = {
                .addr = addr,
                .success = success
            }}
        });
    }

    bool sendMessage(const char* msg) {
        Event event{};
        event.type = Event::Message;
        std::memset(event.data.message.message, 0, sizeof(event.data.message));
        if (msg) {
            std::strncpy(event.data.message.message, msg, sizeof(event.data.message) - 1);
        }
        return outgoingQueue.try_push(event);
    }

};

#endif
