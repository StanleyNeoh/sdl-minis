#include "pong.hpp"
#include "imgui.h"
#include "app/app.hpp"

namespace Pong {
    Pong pong;

    void Pong::process_setup() {
        paddle_update = false;
    }

    void Pong::process_sdl_event(const SDL_Event& event) {
        bool is_master = App::app.role_type == P2P::RoleType_Master;
        auto& paddle = is_master ? rightP : leftP;
        auto& other_paddle = !is_master ? rightP : leftP;
        switch (event.type) {
            case SDL_KEYDOWN: {
                auto key = event.key.keysym.sym;
                switch (key) {
                    case SDLK_w:
                        paddle.move(-20.0);
                        paddle_update = true;
                        break;
                    case SDLK_s:
                        paddle.move(20.0);
                        paddle_update = true;
                        break;
                    case SDLK_UP:
                        other_paddle.move(-20.0);
                        paddle_update = true;
                        break;
                    case SDLK_DOWN:
                        other_paddle.move(20.0);
                        paddle_update = true;
                        break;
                    case SDLK_SPACE:
                        paddle.shoot(!is_master);
                        paddle_update = true;
                        break;
                    case SDLK_RSHIFT:
                        other_paddle.shoot(is_master);
                        paddle_update = true;
                        break;
                    default:
                        break;
                }
                break;
            }
            case SDL_KEYUP: {
                auto key = event.key.keysym.sym;
                const Uint8* state = SDL_GetKeyboardState(NULL);
                switch (key) {
                    case SDLK_w:
                    case SDLK_s: {
                        if (state[SDL_SCANCODE_W] || state[SDL_SCANCODE_S]) break;
                        paddle.move(0);
                        paddle_update = true;
                        break;
                    }
                    case SDLK_UP:
                    case SDLK_DOWN:
                        if (state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_DOWN]) break;
                        other_paddle.move(0);
                        paddle_update = true;
                        break;
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }

    void Pong::process_packet(P2P::Packet& packet) {
        static MetaP::Callbacks callbacks(
            [&](const P2P::PongConfigBody& pong_config) {
                unpack(pong_config);
                App::app.reset_to_state(App::AppState_Pong);
            },
            [&](const P2P::PongPaddleBody& pong_paddle) {
                auto& paddle = App::app.role_type == P2P::RoleType_Master ? leftP : rightP;
                paddle.unpack(pong_paddle);
            },
            [&](const P2P::PongBallBody& pong_ball) {
                ball.unpack(pong_ball);
            }
        );
        callbacks.dispatch<
            MetaP::TT_TVIsEquals<P2P::TV_BodyType>::type,
            MetaP::TO_VariantCast
        >(packet.type, packet.body);
    }

    void Pong::process_takedown() {
        bool is_master = (
            App::app.role_type == P2P::RoleType_Master ||
            App::app.role_type == P2P::RoleType_SinglePlayer
        );
        bool is_send = App::app.role_type == P2P::RoleType_Master;

        State state = step(App::app.frame_stopwatch.delta() / 1000.0f);
        switch (state) {
        case State_Right_Wins:
            clientScore++;
            App::app.messages.push_back("Client won!");
            App::app.messages.push_back("Score= " + std::to_string(clientScore) + " : " + std::to_string(masterScore));
            App::app.reset_to_state(App::AppState_GameSelect);
            break;
        case State_Left_Wins:
            masterScore++;
            App::app.messages.push_back("Master won!");
            App::app.messages.push_back("Score= " + std::to_string(clientScore) + " : " + std::to_string(masterScore));
            App::app.reset_to_state(App::AppState_GameSelect);
            break;
        default:
            break;
        }

        if (is_send) {
            if (paddle_update) {
                auto& paddle = is_master ? rightP : leftP;
                P2P::tcp_manager.outgoingQueue.push(P2P::Packet::create(
                    paddle.pack()
                ));
            }
            if (is_master) {
                P2P::tcp_manager.outgoingQueue.push(P2P::Packet::create(
                    ball.pack()
                ));
            }
        }
    }

    
}
