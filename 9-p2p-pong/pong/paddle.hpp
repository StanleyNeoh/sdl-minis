#ifndef PONG_PADDLE_HPP
#define PONG_PADDLE_HPP

#include "lib/common/common.hpp"
#include "p2p/packet.hpp"

namespace Pong {
    struct Projection {
        Vec2 pos{};
        Vec2 vel{};
        float r = 1.0;
        float life = -1.0;

        void reset() {
            life = -1.0;
        }
    };

    struct Paddle {
        constexpr static int N_PROJ = P2P::PongPaddleBody::N_PROJ;
        constexpr static float VY = 20;
        constexpr static float P_AY = 50;
        constexpr static float P_VY = 30;
        constexpr static float P_VX = 30;

        Vec2 pos;
        float vy = 0;
        float h;
        float proj_ay = 0;
        float proj_vy = 0;

        uint32_t proj_i = 0;
        std::array<Projection, N_PROJ> projs;

        Paddle(float x, float y, float h): pos(x, y), vy(0), h(h) {}

        void move(float vy) {
            this->vy = vy;
            if (vy > 0) {
                proj_ay = P_AY;
            } else if (vy < 0) {
                proj_ay = -P_AY;
            } else {
                proj_ay = 0;
            }
        }

        bool shoot(bool to_right) {
            auto& proj = projs[proj_i];
            if (proj.life >= 0) return false;
            proj.pos.x = pos.x;
            proj.pos.y = pos.y + h / 2;
            proj.life = 1.0;
            proj.vel.x = to_right ? VY : -VY;
            proj.vel.y = proj_vy;
            proj_i = (proj_i + 1) % N_PROJ;
            return true;
        }

        void reset(float x, float y, float _h) {
            pos.x = x;
            pos.y = y;
            h = _h;
            vy = 0;
            proj_vy = 0;
            proj_ay = 0;
            proj_i = 0;
            for (int i = 0; i < N_PROJ; i++) {
                projs[i].reset();
            }
        }

        void step(float dt, float width, float height) {
            {
                constexpr float proj_vy_thresh = 1e-2;
                float _proj_ay = abs(proj_ay) > 1e-6
                    ? proj_ay
                    : abs(proj_vy) < proj_vy_thresh
                    ? 0
                    : proj_vy > 0 
                    ? -Paddle::P_AY
                    : Paddle::P_AY;
                proj_vy = proj_vy + _proj_ay * dt;
                if (proj_vy < -Paddle::P_VY) proj_vy = -Paddle::P_VY;
                else if (proj_vy > Paddle::P_VY) proj_vy = Paddle::P_VY;
                else if (abs(proj_vy) < proj_vy_thresh) proj_vy = 0;
            }
            pos.y = pos.y + vy * dt;
            if (pos.y < 0) {
                pos.y = 0;
                vy = 0;
            } else if (pos.y > height - h) {
                pos.y = height - h;
                vy = 0;
            }
            for (int i = 0; i < N_PROJ; i++) {
                auto& proj = projs[i];
                if (proj.life < 0) continue;
                proj.pos.x += proj.vel.x * dt;
                proj.pos.y += proj.vel.y * dt;
                proj.life -= dt;
                if (proj.pos.y - proj.r < 0) {
                    proj.pos.y = proj.r;
                    if (proj.vel.y < 0) proj.vel.y = -proj.vel.y;
                }
                if (proj.pos.y + proj.r > height) {
                    proj.pos.y = height - proj.r;
                    if (proj.vel.y > 0) proj.vel.y = -proj.vel.y;
                }
            }
        }

        friend std::ostream& operator<<(std::ostream& o, const Paddle& paddle) {
            o << "Paddle{pos=" << paddle.pos << ",h=" << paddle.h << "}";
            return o;
        }

        void unpack(const P2P::PongPaddleBody& padbody) {
            pos.y = padbody.pos_y;
            vy = padbody.vel_y;
            proj_ay = padbody.p_acc_y;
            proj_vy = padbody.p_vel_y;
            proj_i = padbody.proj_i;
            for (int i = 0; i < N_PROJ; i++) {
                projs[i].pos.x = padbody.projs[i].proj_pos_x;
                projs[i].pos.y = padbody.projs[i].proj_pos_y;
                projs[i].vel.x = padbody.projs[i].proj_vel_x;
                projs[i].vel.y = padbody.projs[i].proj_vel_y;
                projs[i].life = padbody.projs[i].proj_life;
            }
        }

        P2P::PongPaddleBody pack() const {
            P2P::PongPaddleBody body{
                .pos_y = pos.y,
                .vel_y = vy,
                .p_acc_y = proj_ay,
                .p_vel_y = proj_vy,
                .proj_i = proj_i
            };
            for (int i = 0; i < N_PROJ; i++) {
                body.projs[i].proj_pos_x = projs[i].pos.x;
                body.projs[i].proj_pos_y = projs[i].pos.y;
                body.projs[i].proj_vel_x = projs[i].vel.x;
                body.projs[i].proj_vel_y = projs[i].vel.y;
                body.projs[i].proj_life = projs[i].life;
            }
            return body;
        }
    };
}

#endif