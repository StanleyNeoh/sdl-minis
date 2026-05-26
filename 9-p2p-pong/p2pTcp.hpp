#ifndef P2PTCP_HPP
#define P2PTCP_HPP

#include <thread>
#include <atomic>
#include <cstring>
#include <SDL.h>
#include "platform_socket.hpp"
#include "pipe.hpp"
#include "logger.hpp"
#include "game.hpp"

namespace P2PTCP {

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
        union {
            struct {
                sockaddr_in addr;
            } connectAction;
            struct {
                sockaddr_in addr;
                bool success;
            } connectEvent;
            struct {
                sockaddr_in addr;
            } disconnectEvent;
            struct {
                SDL_Event event;
            } sdlEvent;
            struct {
                char message[128] = {0};
            } message;
        } data{};

        static Event connection_event(const sockaddr_in& addr, bool success) {
            Event event{};
            event.type = Event::ConnectEvent;
            event.data.connectEvent.addr = addr;
            event.data.connectEvent.success = success;
            return event;
        }

        static Event disconnect_event(const sockaddr_in& addr, bool success) {
            Event event{};
            event.type = Event::DisconnectEvent;
            event.data.disconnectEvent.addr = addr;
            return event;
        }

        static Event message_event(const char* message) {
            Event event{};
            event.type = Event::Message;
            std::memset(event.data.message.message, 0, sizeof(event.data.message));
            if (message) {
                std::strncpy(event.data.message.message, message, sizeof(event.data.message) - 1);
            }
            return event;
        }
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
                received = 0;
                return Recved;
            }
            return NotRecved;
        }
    };

    struct TcpManager {
        using Handler = void(*)(TcpManager*, const sockaddr_in&, bool);

        struct Config {
            u_int32_t port;
            Handler handler;
        };

        const Config config;
        std::thread manager_thread;
        SPSCQueue<Event> incomingQueue;
        SPSCQueue<Event> outgoingQueue;
        bool running; // Used by manager thread only

        static void io_session(TcpManager* ctx, SocketResource socketResource, const sockaddr_in& addr, bool isMaster) {
            Logger logger("TCP IO");
            ctx->incomingQueue.push(Event::connection_event(addr, true));
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
            ctx->incomingQueue.push(Event::disconnect_event(addr, true));
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
                sockaddr_in clientAddr{};
                SocketResource accepted = listener.accept(clientAddr);
                if (accepted.is_available()) {
                    io_session(ctx, std::move(accepted), clientAddr, true);
                    continue;
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
                                ctx->incomingQueue.push(Event::connection_event(event.data.connectAction.addr, false));
                                break;
                            }
                            io_session(ctx, std::move(outbound), event.data.connectAction.addr, false);
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

        bool connect(const sockaddr_in& addr) {
            Event event{};
            event.type = Event::ConnectAction;
            event.data.connectAction.addr = addr;
            return outgoingQueue.try_push(event);
        }

        bool disconnect() {
            Event event{};
            event.type = Event::DisconnectAction;
            return outgoingQueue.try_push(event);
        }

        bool send_message(const char* msg) {
            return outgoingQueue.try_push(
                Event::message_event(msg)
            );
        }

        ~TcpManager() {
            Event event{};
            event.type = Event::KillAction;
            outgoingQueue.push(event);
            manager_thread.join();
        }
    };
}

#endif
