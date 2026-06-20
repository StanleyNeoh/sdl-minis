#ifndef APP_GAME_SELECT_GAME_SELECT_HPP
#define APP_GAME_SELECT_GAME_SELECT_HPP

#include "p2p/type.hpp"
#include "p2p/packet.hpp"
#include "SDL.h"

namespace GameSelect {
    struct GameSelect {
        P2P::GameType my_vote = P2P::GameType_Uninitialized;
        P2P::GameType other_vote = P2P::GameType_Uninitialized;
        int64_t vote_confirm_countdown = -1;

        bool is_initialised = false;

        bool initialise() {
            if (is_initialised) return false;
            return true;
        }

        void reset() {
            vote_confirm_countdown = -1;
            my_vote = P2P::GameType_Uninitialized;
            other_vote = P2P::GameType_Uninitialized;
        }

        void process_setup() {};
        void process_sdl_event(const SDL_Event& event);
        void process_packet(P2P::Packet& packet);

        void draw_checkbox(
            std::string_view id,
            P2P::GameType& vote,
            P2P::GameType game_type,
            bool mine
        );
        void draw();
    };

    extern GameSelect game_select;
}

#endif