#ifndef APP_GAME_SELECT_GAME_SELECT_HPP
#define APP_GAME_SELECT_GAME_SELECT_HPP

#include "p2p/type.hpp"
#include "p2p/packet.hpp"
#include "SDL.h"

namespace GameSelect {
    struct GameSelect {
        P2P::GameType client_vote = P2P::GameType_Uninitialized;
        P2P::GameType master_vote = P2P::GameType_Uninitialized;
        int64_t vote_confirm_countdown = -1;

        bool is_initialised = false;

        bool initialise() {
            if (is_initialised) return false;
            return true;
        }

        void reset() {
            vote_confirm_countdown = -1;
            master_vote = P2P::GameType_Uninitialized;
            client_vote = P2P::GameType_Uninitialized;
        }

        void process_setup() {};
        void process_sdl_event(const SDL_Event& event);
        void process_packet(P2P::Packet& packet);
        void process_takedown() {};

        void draw_checkbox(
            P2P::RoleType role_type,
            bool master_checkbox, 
            P2P::GameType game_type,
            int& id
        );
        void draw();
    };

    extern GameSelect game_select;
}

#endif