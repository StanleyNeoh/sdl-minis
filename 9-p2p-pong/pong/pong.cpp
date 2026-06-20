#include "pong.hpp"
#include "imgui.h"
#include "app/app.hpp"

namespace Pong {
    Pong pong;
    
    bool is_singleplayer() {
        return App::app.role_type == P2P::RoleType_SinglePlayer;
    }

    bool is_left() {
        return (
            App::app.role_type == P2P::RoleType_Master ||
            App::app.role_type == P2P::RoleType_SinglePlayer
        );
    }

    Paddle& my_paddle() {
        return is_left() ? pong.leftP : pong.rightP;
    }

    Paddle& other_paddle() {
        return !is_left() ? pong.leftP : pong.rightP;
    }

    void Pong::process_setup() {
        paddle_update = false;
    }

    void Pong::process_sdl_event(const SDL_Event& event) {
        switch (event.type) {
            case SDL_KEYDOWN: {
                auto key = event.key.keysym.sym;
                switch (key) {
                    case SDLK_w:
                        my_paddle().move(-20.0);
                        paddle_update = true;
                        break;
                    case SDLK_s:
                        my_paddle().move(20.0);
                        paddle_update = true;
                        break;
                    case SDLK_SPACE:
                        my_paddle().shoot(is_left());
                        paddle_update = true;
                        break;
                    default:
                        break;
                }
                if (is_singleplayer()) {
                    switch(key) {
                        case SDLK_UP:
                            other_paddle().move(-20.0);
                            paddle_update = true;
                            break;
                        case SDLK_DOWN:
                            other_paddle().move(20.0);
                            paddle_update = true;
                            break;
                        case SDLK_RSHIFT:
                            other_paddle().shoot(!is_left());
                            paddle_update = true;
                            break;
                    }
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
                        my_paddle().move(0);
                        paddle_update = true;
                        break;
                    }
                    default:
                        break;
                }
                if (is_singleplayer()) {
                    switch (key) {
                    case SDLK_UP:
                    case SDLK_DOWN:
                        if (state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_DOWN]) break;
                        other_paddle().move(0);
                        paddle_update = true;
                        break;
                    }
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
                other_paddle().unpack(pong_paddle);
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
        State state = step(App::app.frame_stopwatch.delta() / 1000.0f);
        switch (state) {
        case State_Right_Wins: {
            ++rightScore;
            Chat::chat.messages.push_back("Right won!");
            Chat::chat.messages.push_back("Score = " + std::to_string(leftScore) + " : " + std::to_string(rightScore));
            App::app.reset_to_state(App::AppState_GameSelect);
            break;
        }
        case State_Left_Wins: {
            ++leftScore;
            Chat::chat.messages.push_back("Left won!");
            Chat::chat.messages.push_back("Score = " + std::to_string(leftScore) + " : " + std::to_string(rightScore));
            App::app.reset_to_state(App::AppState_GameSelect);
            break;
        }
        default:
            break;
        }

        if (!is_singleplayer()) {
            if (paddle_update) {
                P2P::tcp_manager.outgoingQueue.push(P2P::Packet::create(
                    my_paddle().pack()
                ));
            }
            if (is_left()) {
                P2P::tcp_manager.outgoingQueue.push(P2P::Packet::create(
                    ball.pack()
                ));
            }
        }
    }

    
}
