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
            PongState,
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

            struct {
                Pong pong;
            } pongState;
        } data{};
    };

    struct WirePacket {
        std::uint32_t type = 0;
        union {
            struct {
                char message[128];
            } message;
            struct {
                std::uint32_t type;
                std::uint32_t keySym;
            } sdlEvent;
            struct {
                std::uint32_t width;
                std::uint32_t height;
                std::uint32_t pad_w;
                std::uint32_t pad_m;
                std::uint32_t ball_pos_x;
                std::uint32_t ball_pos_y;
                std::uint32_t ball_vel_x;
                std::uint32_t ball_vel_y;
                std::uint32_t ball_r;
                std::uint32_t top_pos_x;
                std::uint32_t top_pos_y;
                std::uint32_t top_w;
                std::uint32_t bot_pos_x;
                std::uint32_t bot_pos_y;
                std::uint32_t bot_w;
            } pongState;
        } data{};

        static constexpr std::size_t HeaderSize = sizeof(type);

        static std::uint32_t encode_u32(std::uint32_t value) {
            return htonl(value);
        }

        static std::uint32_t decode_u32(std::uint32_t value) {
            return ntohl(value);
        }

        static std::uint32_t encode_i32(std::int32_t value) {
            return htonl(static_cast<std::uint32_t>(value));
        }

        static std::int32_t decode_i32(std::uint32_t value) {
            return static_cast<std::int32_t>(ntohl(value));
        }

        static std::uint32_t encode_f32(float value) {
            std::uint32_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            return htonl(bits);
        }

        static float decode_f32(std::uint32_t value) {
            std::uint32_t bits = ntohl(value);
            float decoded = 0.0f;
            std::memcpy(&decoded, &bits, sizeof(decoded));
            return decoded;
        }

        static std::size_t payload_size(Event::Type type) {
            switch (type) {
            case Event::Message:
                return sizeof(data.message);
            case Event::SDLEvent:
                return sizeof(data.sdlEvent);
            case Event::PongState:
                return sizeof(data.pongState);
            default:
                return 0;
            }
        }

        static std::size_t byte_size(Event::Type type) {
            return HeaderSize + payload_size(type);
        }

        std::size_t byte_size() const {
            return byte_size(static_cast<Event::Type>(decode_u32(type)));
        }

        bool encode(const Event& event) {
            *this = {};
            type = encode_u32(static_cast<std::uint32_t>(event.type));
            switch (event.type) {
            case Event::Message:
                std::memcpy(data.message.message, event.data.message.message, sizeof(data.message.message));
                return true;
            case Event::SDLEvent:
                data.sdlEvent.type = encode_u32(static_cast<std::uint32_t>(event.data.sdlEvent.event.type));
                data.sdlEvent.keySym = encode_i32(static_cast<std::int32_t>(event.data.sdlEvent.event.key.keysym.sym));
                return true;
            case Event::PongState: {
                const Pong& pong = event.data.pongState.pong;
                data.pongState.width = encode_f32(pong.width);
                data.pongState.height = encode_f32(pong.height);
                data.pongState.pad_w = encode_f32(pong.pad_w);
                data.pongState.pad_m = encode_f32(pong.pad_m);
                data.pongState.ball_pos_x = encode_f32(pong.ball.pos.x);
                data.pongState.ball_pos_y = encode_f32(pong.ball.pos.y);
                data.pongState.ball_vel_x = encode_f32(pong.ball.vel.x);
                data.pongState.ball_vel_y = encode_f32(pong.ball.vel.y);
                data.pongState.ball_r = encode_f32(pong.ball.r);
                data.pongState.top_pos_x = encode_f32(pong.topP.pos.x);
                data.pongState.top_pos_y = encode_f32(pong.topP.pos.y);
                data.pongState.top_w = encode_f32(pong.topP.w);
                data.pongState.bot_pos_x = encode_f32(pong.botP.pos.x);
                data.pongState.bot_pos_y = encode_f32(pong.botP.pos.y);
                data.pongState.bot_w = encode_f32(pong.botP.w);
                return true;
            }
            default:
                return false;
            }
        }

        bool decode(Event& event) const {
            event = {};
            event.type = static_cast<Event::Type>(decode_u32(type));
            switch (event.type) {
            case Event::Message:
                std::memcpy(event.data.message.message, data.message.message, sizeof(event.data.message.message));
                return true;
            case Event::SDLEvent:
                event.data.sdlEvent.event = {};
                event.data.sdlEvent.event.type = decode_u32(data.sdlEvent.type);
                event.data.sdlEvent.event.key.keysym.sym = static_cast<SDL_Keycode>(decode_i32(data.sdlEvent.keySym));
                return true;
            case Event::PongState:
                event.data.pongState.pong.width = decode_f32(data.pongState.width);
                event.data.pongState.pong.height = decode_f32(data.pongState.height);
                event.data.pongState.pong.pad_w = decode_f32(data.pongState.pad_w);
                event.data.pongState.pong.pad_m = decode_f32(data.pongState.pad_m);
                event.data.pongState.pong.ball.pos.x = decode_f32(data.pongState.ball_pos_x);
                event.data.pongState.pong.ball.pos.y = decode_f32(data.pongState.ball_pos_y);
                event.data.pongState.pong.ball.vel.x = decode_f32(data.pongState.ball_vel_x);
                event.data.pongState.pong.ball.vel.y = decode_f32(data.pongState.ball_vel_y);
                event.data.pongState.pong.ball.r = decode_f32(data.pongState.ball_r);
                event.data.pongState.pong.topP.pos.x = decode_f32(data.pongState.top_pos_x);
                event.data.pongState.pong.topP.pos.y = decode_f32(data.pongState.top_pos_y);
                event.data.pongState.pong.topP.w = decode_f32(data.pongState.top_w);
                event.data.pongState.pong.botP.pos.x = decode_f32(data.pongState.bot_pos_x);
                event.data.pongState.pong.botP.pos.y = decode_f32(data.pongState.bot_pos_y);
                event.data.pongState.pong.botP.w = decode_f32(data.pongState.bot_w);
                return true;
            default:
                return false;
            }
        }
    };

    struct EventReader {
        enum Status {
            Closed,
            Recved,
            NotRecved,
            Invalid
        };

        ssize_t received = 0;
        char buffer[sizeof(WirePacket)];

        std::size_t expected_size() const {
            if (received < static_cast<ssize_t>(sizeof(std::uint32_t))) {
                return sizeof(std::uint32_t);
            }
            std::uint32_t encodedType = 0;
            std::memcpy(&encodedType, buffer, sizeof(encodedType));
            Event::Type type = static_cast<Event::Type>(WirePacket::decode_u32(encodedType));
            return WirePacket::byte_size(type);
        }

        Status recv(const SocketResource& socketResource, Event& event) {
            std::size_t targetSize = expected_size();
            if (targetSize == 0 || targetSize > sizeof(WirePacket)) {
                received = 0;
                return Invalid;
            }
            ssize_t newReceived = socketResource.recv(buffer + received, targetSize - received);
            if (newReceived == 0) return Closed;
            if (newReceived < 0) return NotRecved;
            received += newReceived;
            targetSize = expected_size();
            if (targetSize == 0 || targetSize > sizeof(WirePacket)) {
                received = 0;
                return Invalid;
            }
            if (received == static_cast<ssize_t>(targetSize)) {
                WirePacket packet{};
                std::memcpy(&packet, buffer, targetSize);
                if (!packet.decode(event)) {
                    received = 0;
                    return Invalid;
                }
                event.fromPeer = true;
                received = 0;
                return Recved;
            }
            return NotRecved;
        }
    };

    struct EventWriter {
        enum Status {
            Closed,
            Sent,
            NotSent,
            Idle
        };

        ssize_t sent = 0;
        bool hasPending = false;
        std::size_t packetSize = 0;
        WirePacket packet{};

        void queue(const Event& newEvent) {
            hasPending = packet.encode(newEvent);
            sent = 0;
            packetSize = hasPending ? packet.byte_size() : 0;
        }

        Status send(const SocketResource& socketResource) {
            if (!hasPending) return Idle;
            const char* bytes = reinterpret_cast<const char*>(&packet);
            ssize_t newSent = socketResource.send(bytes + sent, packetSize - sent);
            if (newSent == 0) return Closed;
            if (newSent < 0) return NotSent;
            sent += newSent;
            if (sent == static_cast<ssize_t>(packetSize)) {
                sent = 0;
                packetSize = 0;
                hasPending = false;
                return Sent;
            }
            return NotSent;
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
        EventWriter writer;
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
            case EventReader::Invalid:
                logger.log("Received invalid wire packet");
                disconnected = true;
                break;
            default:
                break;
            }

            auto writeStatus = writer.send(socketResource);
            switch (writeStatus) {
            case EventWriter::Closed:
                disconnected = true;
                break;
            default:
                break;
            }

            while (!writer.hasPending && ctx->outgoingQueue.try_pop(event)) {
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
                    writer.queue(event);
                    if (!writer.hasPending) {
                        logger.log("Dropped unsupported wire event type: ", static_cast<int>(event.type));
                    }
                    break;
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
