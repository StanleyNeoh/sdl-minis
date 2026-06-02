#ifndef PACKET_HPP
#define PACKET_HPP
#include "platform_socket.hpp"

namespace Packet {
    enum Type: uint32_t {
        UninitializedType,
        ConnectRequestType,
        ConnectResponseType,
        DisconnectRequestType,
        DisconnectResponseType,
        KillRequestType,
        MessageType,
        PongReadyType,
        PongConfigType,
        PongPaddleType,
        PongBallType,
    };

    struct ConnectRequestPacket {
        constexpr static Type type = ConnectRequestType;
        constexpr static bool is_wireable = false;
        sockaddr_in addr;
    };

    struct ConnectResponsePacket {
        constexpr static Type type = ConnectResponseType;
        constexpr static bool is_wireable = false;
        sockaddr_in addr;
        bool is_master;
    };

    struct DisconnectRequestPacket {
        constexpr static Type type = DisconnectRequestType;
        constexpr static bool is_wireable = false;
    };

    struct DisconnectResponsePacket {
        constexpr static Type type = DisconnectResponseType;
        constexpr static bool is_wireable = false;
        sockaddr_in addr;
    };

    struct MessagePacket {
        constexpr static Type type = MessageType;
        constexpr static bool is_wireable = true;
        char message[128] = {0};

        void serialize(char* buf) const {
            memcpy(buf, message, sizeof(message));
        }

        void deserialize(char* buf) {
            memcpy(message, buf, sizeof(message));
        }

        static constexpr size_t size() {
            return sizeof(message);
        }
    };

    struct PongReadyPacket {
        constexpr static Type type = PongReadyType;
        constexpr static bool is_wireable = true;
        bool ready;

        void serialize(char* buf) const {
            memcpy(buf, &ready, sizeof(ready));
        }

        void deserialize(char* buf) {
            memcpy(&ready, buf, sizeof(ready));
        }

        static constexpr size_t size() {
            return sizeof(ready);
        }
    };

    struct PongConfigPacket {
        constexpr static Type type = PongConfigType;
        constexpr static bool is_wireable = true;
        float width;
        float height;
        float pad_w;
        float pad_m;
        float ball_pos_x;
        float ball_pos_y;
        float ball_vel_x;
        float ball_vel_y;
        float ball_r;
        float top_pos_x;
        float top_pos_y;
        float top_w;
        float bot_pos_x;
        float bot_pos_y;
        float bot_w;

        void serialize(char* buf) const {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float x) {
                float y = encode_f32(x);
                memcpy(ptr, &y, sizeof(y));
                ptr += sizeof(x);
            };
            mmemcpy(ptr, width);
            mmemcpy(ptr, height);
            mmemcpy(ptr, pad_w);
            mmemcpy(ptr, pad_m);
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
            mmemcpy(ptr, ball_r);
            mmemcpy(ptr, top_pos_x);
            mmemcpy(ptr, top_pos_y);
            mmemcpy(ptr, top_w);
            mmemcpy(ptr, bot_pos_x);
            mmemcpy(ptr, bot_pos_y);
            mmemcpy(ptr, bot_w);
        }

        void deserialize(char* buf) {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float& x) {
                uint32_t y;
                memcpy(&y, ptr, sizeof(x));
                x = decode_f32(y);
                ptr += sizeof(x);
            };
            mmemcpy(ptr, width);
            mmemcpy(ptr, height);
            mmemcpy(ptr, pad_w);
            mmemcpy(ptr, pad_m);
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
            mmemcpy(ptr, ball_r);
            mmemcpy(ptr, top_pos_x);
            mmemcpy(ptr, top_pos_y);
            mmemcpy(ptr, top_w);
            mmemcpy(ptr, bot_pos_x);
            mmemcpy(ptr, bot_pos_y);
            mmemcpy(ptr, bot_w);
        }

        static constexpr size_t size() {
            return sizeof(PongConfigPacket) / sizeof(float) * sizeof(uint32_t);
        }
    };

    struct PongPaddlePacket {
        constexpr static Type type = PongPaddleType;
        constexpr static bool is_wireable = true;
        float pos_x;
        float vel_x;

        void serialize(char* buf) const {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float x) {
                float y = encode_f32(x);
                memcpy(ptr, &y, sizeof(y));
                ptr += sizeof(x);
            };
            mmemcpy(ptr, pos_x);
            mmemcpy(ptr, vel_x);
        }

        void deserialize(char* buf) {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float& x) {
                uint32_t y;
                memcpy(&y, ptr, sizeof(x));
                x = decode_f32(y);
                ptr += sizeof(x);
            };
            mmemcpy(ptr, pos_x);
            mmemcpy(ptr, vel_x);
        }

        static constexpr size_t size() {
            return sizeof(PongPaddlePacket) / sizeof(float) * sizeof(uint32_t);
        }
    };

    struct PongBallPacket {
        constexpr static Type type = PongBallType;
        constexpr static bool is_wireable = true;
        float ball_pos_x;
        float ball_pos_y;
        float ball_vel_x;
        float ball_vel_y;

        void serialize(char* buf) const {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float x) {
                float y = encode_f32(x);
                memcpy(ptr, &y, sizeof(y));
                ptr += sizeof(x);
            };
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
        }

        void deserialize(char* buf) {
            char* ptr = buf;
            auto mmemcpy = [](char*& ptr, float& x) {
                uint32_t y;
                memcpy(&y, ptr, sizeof(x));
                x = decode_f32(y);
                ptr += sizeof(x);
            };
            mmemcpy(ptr, ball_pos_x);
            mmemcpy(ptr, ball_pos_y);
            mmemcpy(ptr, ball_vel_x);
            mmemcpy(ptr, ball_vel_y);
        }

        static constexpr size_t size() {
            return sizeof(PongBallPacket) / sizeof(float) * sizeof(uint32_t);
        }
    };

    struct Packet {
        Type type = UninitializedType;
        union {
            ConnectRequestPacket connect_request;
            ConnectResponsePacket connect_response;
            DisconnectRequestPacket disconnect_request;
            DisconnectResponsePacket disconnect_response;
            MessagePacket message;
            PongConfigPacket pong_config;
            PongReadyPacket pong_ready;
            PongPaddlePacket pong_paddle;
            PongBallPacket pong_ball;
        } data{};
    };

    // Helper functions for packet type checking
    inline bool is_wireable(Type type) {
        return type == MessageType || type == PongReadyType || 
               type == PongConfigType || type == PongPaddleType || 
               type == PongBallType;
    }

    inline bool is_disconnect(Type type) {
        return type == DisconnectRequestType || type == KillRequestType;
    }
}

// Variadic template metaprogramming - Forward declarations (must be outside namespace)
template <typename A, typename... Rest>
constexpr size_t get_size(Packet::Type t);

template <typename A, typename... Rest>
bool serialize_packet(Packet::Type t, const Packet::Packet& packet, char* buf);

template <typename A, typename... Rest>
bool deserialize_packet(Packet::Type t, Packet::Packet& packet, char* buf);

// Convenience wrappers declarations
size_t packet_size(Packet::Type t);
bool packet_serialize(Packet::Type t, const Packet::Packet& packet, char* buf);
bool packet_deserialize(Packet::Type t, Packet::Packet& packet, char* buf);

namespace Packet {
    struct Reader {
        enum Status {
            Closed,
            Recved,
            NotRecved,
            Invalid
        };

        Type type = UninitializedType;
        ssize_t target_size = sizeof(Type);
        ssize_t received = 0;
        char buffer[sizeof(Packet)];

        void reset() {
            type = UninitializedType;
            target_size = sizeof(Type);
            received = 0;
        }

        Status recv(const SocketResource& socketResource, Packet& packet) {
            ssize_t newReceived = socketResource.recv(buffer + received, target_size - received);
            if (newReceived == 0) return Closed;
            if (newReceived < 0) return NotRecved;
            received += newReceived;
            
            if (received == target_size) {
                if (type == UninitializedType) {
                    // Decode packet type
                    uint32_t encodedType = 0;
                    std::memcpy(&encodedType, buffer, sizeof(encodedType));
                    packet.type = type = static_cast<Type>(ntohl(encodedType));
                    
                    // Use variadic template to get size
                    target_size = ::packet_size(type);
                    if (target_size == 0) {
                        reset();
                        return Invalid;
                    }
                    // Reset received counter to read body from start of buffer
                    received = 0;
                    return NotRecved;
                } else {
                    // Use variadic template to deserialize
                    if (::packet_deserialize(type, packet, buffer)) {
                        reset();
                        return Recved;
                    } else {
                        reset();
                        return Invalid;
                    }
                }
            }
            return NotRecved;
        }
    };
    
    struct Writer {
        char buffer[sizeof(Packet)];
        
        size_t serialize(const Packet& packet) {
            // Encode type in network byte order
            uint32_t encodedType = htonl(static_cast<uint32_t>(packet.type));
            std::memcpy(buffer, &encodedType, sizeof(encodedType));
            
            // Use variadic template to serialize body
            if (::packet_serialize(packet.type, packet, buffer + sizeof(Type))) {
                return sizeof(Type) + ::packet_size(packet.type);
            }
            return sizeof(Type);
        }
        
        bool send(const SocketResource& socketResource, const Packet& packet) {
            size_t packetSize = serialize(packet);
            return socketResource.send_exact(buffer, packetSize);
        }
    };
}

// Template specializations for packet size
template<typename PacketType>
constexpr size_t get_packet_size();
template<> constexpr size_t get_packet_size<Packet::MessagePacket>() { 
    return sizeof(Packet::MessagePacket::message); 
}
template<> constexpr size_t get_packet_size<Packet::PongReadyPacket>() { 
    return sizeof(Packet::PongReadyPacket::ready); 
}
template<> constexpr size_t get_packet_size<Packet::PongConfigPacket>() { 
    return sizeof(Packet::PongConfigPacket) / sizeof(float) * sizeof(uint32_t); 
}
template<> constexpr size_t get_packet_size<Packet::PongPaddlePacket>() { 
    return sizeof(Packet::PongPaddlePacket) / sizeof(float) * sizeof(uint32_t); 
}
template<> constexpr size_t get_packet_size<Packet::PongBallPacket>() { 
    return sizeof(Packet::PongBallPacket) / sizeof(float) * sizeof(uint32_t); 
}

template<typename PacketType>
inline auto& get_union_member(Packet::Packet& packet);
template<> inline auto& get_union_member<Packet::MessagePacket>(Packet::Packet& p) {
    return p.data.message;
}
template<> inline auto& get_union_member<Packet::PongReadyPacket>(Packet::Packet& p) {
    return p.data.pong_ready;
}
template<> inline auto& get_union_member<Packet::PongConfigPacket>(Packet::Packet& p) {
    return p.data.pong_config;
}
template<> inline auto& get_union_member<Packet::PongPaddlePacket>(Packet::Packet& p) {
    return p.data.pong_paddle;
}
template<> inline auto& get_union_member<Packet::PongBallPacket>(Packet::Packet& p) {
    return p.data.pong_ball;
}

template <typename A, typename... Rest>
constexpr size_t get_size(Packet::Type t) {
    if (t == A::type) {
        return get_packet_size<A>();
    }
    if constexpr (sizeof...(Rest) > 0) {
        return get_size<Rest...>(t);
    }
    return 0;
}

// Serialize with variadic dispatch
template <typename A, typename... Rest>
bool serialize_packet(Packet::Type t, const Packet::Packet& packet, char* buf) {
    if (t == A::type) {
        auto& member = get_union_member<A>(const_cast<Packet::Packet&>(packet));
        member.serialize(buf);
        return true;
    }
    if constexpr (sizeof...(Rest) > 0) {
        return serialize_packet<Rest...>(t, packet, buf);
    }
    return false;
}

// Deserialize with variadic dispatch  
template <typename A, typename... Rest>
bool deserialize_packet(Packet::Type t, Packet::Packet& packet, char* buf) {
    if (t == A::type) {
        auto& member = get_union_member<A>(packet);
        member.deserialize(buf);
        return true;
    }
    if constexpr (sizeof...(Rest) > 0) {
        return deserialize_packet<Rest...>(t, packet, buf);
    }
    return false;
}

// Convenience wrapper using all wireable packet types
inline size_t packet_size(Packet::Type t) {
    return get_size<
        Packet::MessagePacket,
        Packet::PongReadyPacket,
        Packet::PongConfigPacket,
        Packet::PongPaddlePacket,
        Packet::PongBallPacket
    >(t);
}

inline bool packet_serialize(Packet::Type t, const Packet::Packet& packet, char* buf) {
    return serialize_packet<
        Packet::MessagePacket,
        Packet::PongReadyPacket,
        Packet::PongConfigPacket,
        Packet::PongPaddlePacket,
        Packet::PongBallPacket
    >(t, packet, buf);
}

inline bool packet_deserialize(Packet::Type t, Packet::Packet& packet, char* buf) {
    return deserialize_packet<
        Packet::MessagePacket,
        Packet::PongReadyPacket,
        Packet::PongConfigPacket,
        Packet::PongPaddlePacket,
        Packet::PongBallPacket
    >(t, packet, buf);
}

#endif