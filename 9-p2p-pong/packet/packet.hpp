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
    using BodyList = MetaP::TD_List<
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

    using _Variant = MetaP::Variant<BodyList>;
    struct Packet {
        Type type = UninitializedType;
        _Variant body;

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
                _Variant
            >::f(body, type, buffer);
        }

        bool deserialize_body(Type type, const char* buffer) {
            return MetaP::TO_VariantDispatch<
                TV_IsWireable, 
                MetaP::TT_TVIsEquals<TV_BodyType>::type, 
                TO_Deserialize, 
                _Variant
            >::f(body, type, buffer);
        }

        static size_t body_size(Type type) {
            return MetaP::TO_ListDispatch<
                MetaP::TT_TVIsEquals<TV_BodyType>::type, 
                MetaP::TT_TVToTO<TV_BodySize>::type, 
                BodyList
            >::f(type);
        }

        bool is_wireable() {
            return MetaP::TO_ListDispatch<
                MetaP::TT_TVIsEquals<TV_BodyType>::type, 
                MetaP::TT_TVToTO<TV_IsWireable>::type, 
                BodyList
            >::f(type);
        }

        bool is_disconnect() {
            return MetaP::TO_ListDispatch<
                MetaP::TT_TVIsEquals<TV_BodyType>::type, 
                MetaP::TT_TVToTO<TV_IsDisconnect>::type, 
                BodyList
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
}

#endif

