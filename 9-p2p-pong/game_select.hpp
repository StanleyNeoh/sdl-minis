#ifndef GAME_SELECT_HPP
#define GAME_SELECT_HPP

#include "p2p/type.hpp"
#include "pong.hpp"

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

        void draw_checkbox(
            P2P::RoleType role_type,
            bool master_checkbox, 
            P2P::GameType game_type,
            int& id
        );

        void draw(P2P::RoleType role_type, Pong& pong_ref);
    };

    extern GameSelect game_select;
}

#endif