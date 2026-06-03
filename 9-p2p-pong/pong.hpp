#ifndef PONG_HPP
#define PONG_HPP

#include <algorithm>
#include <iostream>
#include "utils.hpp"
#include "packet/packet.hpp"

struct Pong {
    enum State {
        State_Ongoing,
        State_Top_Wins,
        State_Bot_Wins,
    };

    struct Paddle {
        Vec2 pos;
        float vx = 0;
        float w;

        Paddle(float x, float y, float w): pos(x, y), vx(0), w(w) {}

        void move(float vx) {
            this->vx = vx;
        }

        void reset(float x, float y, float w) {
            pos.x = x;
            pos.y = y;
            w = w;
        }

        friend std::ostream& operator<<(std::ostream& o, const Paddle& paddle) {
            o << "Paddle{pos=" << paddle.pos << ",w=" << paddle.w << "}";
            return o;
        }

        void unpack(const Packet::PongPaddleBody& padbody) {
            pos.x = padbody.pos_x;
            vx = padbody.vel_x;
        }

        Packet::PongPaddleBody pack() const {
            return Packet::PongPaddleBody{
                .pos_x = pos.x,
                .vel_x = vx
            };
        }
    };

    struct Ball {
        Vec2 pos;
        Vec2 vel;
        float r;
        Ball(float x, float y, float r = 1.0, float v = 10.0): pos(x, y), vel(Vec2::rand_unit(v)), r(r) {}

        struct OverlapInfo {
            Vec2 normal;
        };

        bool overlap_pt(float x, float y, OverlapInfo& info) const {
            float dx = pos.x - x;
            float dy = pos.y - y;
            float d2 = dx * dx + dy * dy;
            if (d2 > r * r) return false;
            info.normal = Vec2(dx, dy);
            info.normal.normalise();
            return true;
        }

        bool overlap_hsec(float x, float y, float l, OverlapInfo& info) const {
            if (pos.x < x || pos.x > x + l || abs(pos.y - y) > r) return false;
            info.normal = pos.y > y ? Vec2(0, 1) : Vec2(0, -1);
            return true;
        }

        bool overlap_paddle(const Paddle& pad, OverlapInfo& info) const {
            return (
                overlap_pt(pad.pos.x, pad.pos.y, info) 
                || overlap_pt(pad.pos.x + pad.w, pad.pos.y, info)
                || overlap_hsec(pad.pos.x, pad.pos.y, pad.w, info)
            );
        }

        void reset(float x, float y, float v = 10.0) {
            pos.x = x;
            pos.y = y;
            vel = Vec2::rand_unit(v);
        }

        void reset(float x, float y, float vx, float vy) {
            pos.x = x;
            pos.y = y;
            vel.x = vx;
            vel.y = vy;
        }

        friend std::ostream& operator<<(std::ostream& o, const Ball& ball) {
            o << "Ball{pos=" << ball.pos << ",vel=" << ball.vel << ",r=" << ball.r<< "}";
            return o;
        }

        void unpack(const Packet::PongBallBody& ballbody)  {
            reset(
                ballbody.ball_pos_x,
                ballbody.ball_pos_y,
                ballbody.ball_vel_x,
                ballbody.ball_vel_y
            );
        }

        Packet::PongBallBody pack() const {
            return Packet::PongBallBody{
                .ball_pos_x = pos.x,
                .ball_pos_y = pos.y,
                .ball_vel_x = vel.x,
                .ball_vel_y = vel.y
            };
        }
    };

    float width;
    float height;
    float pad_w;
    float pad_m;
    Ball ball;
    Paddle topP;
    Paddle botP;

    Pong(float width = 30.0, float height = 30.0, float ball_r = 1.0, float pad_w = 3.0, float pad_m = 1.0): 
        width(width), 
        height(height),
        pad_w(pad_w),
        pad_m(pad_m),
        ball(width / 2, height / 2, ball_r),
        topP(width / 2 - pad_w / 2, pad_m, pad_w),
        botP(width / 2 - pad_w / 2, height - pad_m, pad_w)
    {}

    void reset() {
        ball.reset(width / 2, height / 2);
        topP.reset(width / 2 - pad_w / 2, pad_m, pad_w);
        botP.reset(width / 2 - pad_w / 2, height - pad_m, pad_w);
    }

    State step(float dt) {
        ball.pos.x = ball.pos.x + ball.vel.x * dt;
        ball.pos.y = ball.pos.y + ball.vel.y * dt;
        topP.pos.x = SDL_clamp(topP.pos.x + topP.vx * dt, 0, width - topP.w);
        botP.pos.x = SDL_clamp(botP.pos.x + botP.vx * dt, 0, width - botP.w);
        if (ball.pos.x - ball.r < 0) {
            ball.pos.x = ball.r;
            if (ball.vel.x < 0) ball.vel.x = -ball.vel.x;
        }
        if (ball.pos.y - ball.r < 0) {
            return State_Bot_Wins;
        }
        if (ball.pos.x + ball.r > width) {
            ball.pos.x = width - ball.r;
            if (ball.vel.x > 0) ball.vel.x = -ball.vel.x;
        }
        if (ball.pos.y + ball.r > height) {
            return State_Top_Wins;
        }
        Ball::OverlapInfo info;
        if (ball.overlap_paddle(topP, info) && info.normal.dot(ball.vel) < 0) {
            float scale = ball.vel.x * info.normal.x + ball.vel.y * info.normal.y;
            ball.vel.x -= info.normal.x * 2 * scale;
            ball.vel.y -= info.normal.y * 2 * scale;
        }
        if (ball.overlap_paddle(botP, info) && info.normal.dot(ball.vel) < 0) {
            float scale = ball.vel.x * info.normal.x + ball.vel.y * info.normal.y;
            ball.vel.x -= info.normal.x * 2 * scale;
            ball.vel.y -= info.normal.y * 2 * scale;
        }
        return State_Ongoing;
    }

    friend std::ostream& operator<<(std::ostream& o, const Pong& pong) {
        o << "Pong{\n" 
            << "  Top" << pong.topP << "\n"
            << "  Bot" << pong.botP << "\n"
            << "  " << pong.ball << "\n"
            << "}";
        return o;
    }

    void unpack(const Packet::PongConfigBody& config) {
        width = config.width;
        height = config.height;
        pad_w = config.pad_w;
        pad_m = config.pad_m;
        ball.reset(
            config.ball_pos_x, 
            config.ball_pos_y, 
            config.ball_vel_x, 
            config.ball_vel_y
        );
        ball.r = config.ball_r;
        topP.reset(
            config.top_pos_x,
            config.top_pos_y,
            pad_w
        );
        botP.reset(
            config.bot_pos_x,
            config.bot_pos_y,
            pad_w
        );
    }

    Packet::PongConfigBody pack() const {
        return Packet::PongConfigBody{
            .width = width,
            .height = height,
            .pad_w = pad_w,
            .pad_m = pad_m,
            .ball_pos_x = ball.pos.x,
            .ball_pos_y = ball.pos.y,
            .ball_vel_x = ball.vel.x,
            .ball_vel_y = ball.vel.y,
            .ball_r = ball.r,
            .top_pos_x = topP.pos.x,
            .top_pos_y = topP.pos.y,
            .top_w = topP.w,
            .bot_pos_x = botP.pos.x,
            .bot_pos_y = botP.pos.y,
            .bot_w = botP.w
        };
    }

};

#endif