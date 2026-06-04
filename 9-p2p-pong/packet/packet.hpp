#ifndef PACKET_PACKET_HPP
#define PACKET_PACKET_HPP

#include <cstring>
#include <new>
#include "../metap/metap.hpp"
#include "../platform_socket.hpp"
#include "type.hpp"

#include "body/body_traits.hpp"
#include "body/uninitialized.hpp"
#include "body/connect_request.hpp"
#include "body/connect_response.hpp"
#include "body/disconnect_request.hpp"
#include "body/disconnect_response.hpp"
#include "body/kill_request.hpp"
#include "body/message.hpp"
#include "body/pong_config.hpp"
#include "body/pong_ready.hpp"
#include "body/pong_paddle.hpp"
#include "body/pong_ball.hpp"

namespace Packet {
    using BodyRegistry = MetaP::TD_List<
        UninitializedBody,
        ConnectRequestBody,
        ConnectResponseBody,
        DisconnectRequestBody,
        DisconnectResponseBody,
        KillRequestBody,
        MessageBody,
        PongConfigBody,
        PongReadyBody,
        PongPaddleBody,
        PongBallBody
    >;

    using Variant = MetaP::VariantL<BodyRegistry>;
    struct Packet {
        Type type = UninitializedType;
        Variant body;

        template <typename _Body>
        static Packet create(_Body&& body) {
            // if body is Lvalue, _Body = T&
            // if body is Rvalue, _Body = T
            using CleanBody = std::remove_reference_t<_Body>;
            Packet packet;
            packet.type = TV_BodyType<CleanBody>::value;
            new (&packet.body) CleanBody(std::forward<_Body>(body));
            return packet;
        }

        bool serialize_body(char* buffer) const {
            return MetaP::TO_VariantDispatch<
                TV_IsWireable, 
                MetaP::TT_TVIsEquals<TV_BodyType>::type, 
                TO_Serialize,
                Variant
            >::f(body, type, buffer);
        }

        bool deserialize_body(Type type, const char* buffer) {
            return MetaP::TO_VariantDispatch<
                TV_IsWireable, 
                MetaP::TT_TVIsEquals<TV_BodyType>::type, 
                TO_Deserialize, 
                Variant
            >::f(body, type, buffer);
        }

        static size_t body_size(Type type) {
            return MetaP::TO_Dispatch<
                MetaP::TT_TVIsEquals<TV_BodyType>::type, 
                MetaP::TT_TVToTO<TV_BodySize>::type, 
                BodyRegistry
            >::f(type);
        }

        bool is_wireable() {
            return MetaP::TO_Dispatch<
                MetaP::TT_TVIsEquals<TV_BodyType>::type, 
                MetaP::TT_TVToTO<TV_IsWireable>::type, 
                BodyRegistry
            >::f(type);
        }

        bool is_disconnect() {
            return MetaP::TO_Dispatch<
                MetaP::TT_TVIsEquals<TV_BodyType>::type, 
                MetaP::TT_TVToTO<TV_IsDisconnect>::type, 
                BodyRegistry
            >::f(type);
        }

        bool send(const SocketResource& socketResource) {
            Logger logger("Sender");
            // Encode type in network byte order
            char buffer[sizeof(Packet)];
            uint32_t encodedType = htonl(static_cast<uint32_t>(type));
            std::memcpy(buffer, &encodedType, sizeof(encodedType));
            
            if (!serialize_body(buffer + sizeof(Type))) return false;
            return socketResource.send_exact(buffer, sizeof(Type) + Packet::body_size(type));
        }

        template <typename Body>
        decltype(auto) get() {
            return body.get<Body>();
        }
    };

    namespace Dispatcher { 
        template <typename F>
        struct Handler {
            using FirstArg = typename MetaP::TT_FirstArg<F>::type;
            F func;
            Handler(F&& f): func(std::forward<F>(f)) {}

            template <typename... Args>
            decltype(auto) operator()(Args&&... args) {
                return func(std::forward<Args>(args)...);
            }
        };

        template <
            typename V,
            typename T, 
            typename... Ts
        >
        static bool _dispatch(V&& variant, Type type, T&& first, Ts&&... rest) {
            using BodyType = std::decay_t<typename MetaP::TT_FirstArg<std::decay_t<T>>::type>;
            if (TV_BodyType<BodyType>::value == type) {
                first(std::forward<V>(variant).template get<BodyType>());
                return true;
            }
            if constexpr (sizeof...(Ts) > 0) {
                return _dispatch(std::forward<V>(variant), type, std::forward<Ts>(rest)...);
            }
            return false;
        }

        template <typename... Handlers>
        static bool dispatch(Packet& packet, Handlers&&... handlers) {
            return _dispatch(packet.body, packet.type, std::forward<Handlers>(handlers)...);
        }
    }
}

#endif

