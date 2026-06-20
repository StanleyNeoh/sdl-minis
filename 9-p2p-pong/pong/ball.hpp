#ifndef PONG_BALL_HPP
#define PONG_BALL_HPP

#include "lib/common/common.hpp"
#include "paddle.hpp"

namespace Pong {
    struct Ball {
        Vec2 pos;
        Vec2 vel;
        float r;
        Ball(float x, float y, float r = 1.0, float v = 10.0): pos(x, y), vel(Rand::vec2_real(v)), r(r) {}

        struct OverlapInfo {
            Vec2 normal;
        };

        bool overlap_pt(float x, float y, OverlapInfo& info) const {
            float dx = pos.x - x;
            float dy = pos.y - y;
            float d2 = dx * dx + dy * dy;
            if (d2 > r * r) return false;
            if (d2 == 0.0f) {
                info.normal = Vec2(1.0f, 0.0f);
            } else {
                info.normal = Vec2(dx, dy);
                info.normal.normalise();
            }
            return true;
        }

        bool overlap_vsec(float x, float y, float l, OverlapInfo& info) const {
            if (pos.y < y || pos.y > y + l || abs(pos.x - x) > r) return false;
            info.normal = pos.x > x ? Vec2(1, 0) : Vec2(-1, 0);
            return true;
        }

        bool overlap_paddle(const Paddle& pad, OverlapInfo& info) const {
            return (
                overlap_pt(pad.pos.x, pad.pos.y, info) 
                || overlap_pt(pad.pos.x, pad.pos.y + pad.h, info)
                || overlap_vsec(pad.pos.x, pad.pos.y, pad.h, info)
            );
        }

        bool overlap_proj(const Projection& proj, OverlapInfo& info) const {
            if (proj.life < 0) return false;
            float dx = pos.x - proj.pos.x;
            float dy = pos.y - proj.pos.y;
            float d2 = dx * dx + dy * dy;
            float maxd = proj.r + r;
            if (d2 > maxd * maxd) return false;
            if (d2 == 0.0f) {
                info.normal = Vec2(1.0f, 0.0f);
            } else {
                info.normal.x = dx;
                info.normal.y = dy;
                info.normal.normalise();
            }
            return true;
        }

        void reset(float x, float y, float v = 10.0) {
            pos.x = x;
            pos.y = y;
            vel = Rand::vec2_real(v);
        }

        void reset(float x, float y, float vx, float vy) {
            pos.x = x;
            pos.y = y;
            vel.x = vx;
            vel.y = vy;
        }

        void step(float dt, float width, float height) {
            pos.x = pos.x + vel.x * dt;
            pos.y = pos.y + vel.y * dt;
            if (pos.y - r < 0) {
                pos.y = r;
                if (vel.y < 0) vel.y = -vel.y;
            }
            if (pos.y + r > height) {
                pos.y = height - r;
                if (vel.y > 0) vel.y = -vel.y;
            }
        }

        friend std::ostream& operator<<(std::ostream& o, const Ball& ball) {
            o << "Ball{pos=" << ball.pos << ",vel=" << ball.vel << ",r=" << ball.r<< "}";
            return o;
        }

        void unpack(const P2P::PongBallBody& ballbody)  {
            reset(
                ballbody.ball_pos_x,
                ballbody.ball_pos_y,
                ballbody.ball_vel_x,
                ballbody.ball_vel_y
            );
        }

        P2P::PongBallBody pack() const {
            return P2P::PongBallBody{
                .ball_pos_x = pos.x,
                .ball_pos_y = pos.y,
                .ball_vel_x = vel.x,
                .ball_vel_y = vel.y
            };
        }
    };
}

#endif