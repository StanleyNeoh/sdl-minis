#ifndef P2P_READER_HPP
#define P2P_READER_HPP

#include "packet.hpp"

namespace P2P {
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
                    target_size = Packet::body_size(type);
                    if (target_size == 0) {
                        reset();
                        return Invalid;
                    }
                    // Reset received counter to read body from start of buffer
                    received = 0;
                    return NotRecved;
                } else {
                    // Use variadic template to deserialize
                    packet.deserialize_body(type, buffer);
                    reset();
                    return Recved;
                }
            }
            return NotRecved;
        }

    };
}

#endif