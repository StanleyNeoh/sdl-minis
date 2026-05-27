#ifndef PONG_HPP
#define PONG_HPP

#include <iostream>
#include "utils.hpp"

struct Pong {
    enum State {
        State_Ongoing,
        State_Top_Wins,
        State_Bot_Wins,
    };

    struct Paddle {
        Vec2 pos;
        float w;

        Paddle(float x, float y, float w): pos(x, y), w(w) {}

        void move(float dx) {
            pos.x += dx;
        }

        void reset(float x, float y) {
            pos.x = x;
            pos.x = y;
        }

        friend std::ostream& operator<<(std::ostream& o, const Paddle& paddle) {
            o << "Paddle{pos=" << paddle.pos << ",w=" << paddle.w << "}";
            return o;
        }
    };

    struct Ball {
        Vec2 pos;
        Vec2 vel;
        float r;
        Ball(float x, float y, float r = 1.0): pos(x, y), vel(Vec2::rand_unit()), r(r) {}

        struct OverlapInfo {
            Vec2 normal;
        };

        bool overlap_pt(float x, float y, OverlapInfo& info) const {
            float dx = pos.x - x;
            float dy = pos.y - y;
            float d2 = dx * dx + dy * dy;
            if (r * r > d2) return false;
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

        void reset(float x, float y) {
            pos.x = x;
            pos.y = y;
            vel = Vec2::rand_unit();
        }

        friend std::ostream& operator<<(std::ostream& o, const Ball& ball) {
            o << "Ball{pos=" << ball.pos << ",vel=" << ball.vel << ",r=" << ball.r<< "}";
            return o;
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
        ball(width / 2, height / 2),
        topP(width - pad_w / 2, pad_m, pad_w),
        botP(width - pad_w / 2, height - pad_m, pad_w)
    {}

    void reset() {
        ball.reset(width / 2, height / 2);
        topP.reset(width - pad_w / 2, pad_m);
        botP.reset(width - pad_w / 2, height - pad_m);
    }

    State step(float dt) {
        ball.pos.x = ball.pos.x + ball.vel.x * dt;
        ball.pos.y = ball.pos.y + ball.vel.y * dt;
        if (ball.pos.x < 0) {
            ball.pos.x = 0;
            if (ball.vel.x < 0) ball.vel.x = -ball.vel.x;
        }
        if (ball.pos.y < 0) {
            return State_Bot_Wins;
        }
        if (ball.pos.x > width) {
            ball.pos.x = width;
            if (ball.vel.x > 0) ball.vel.x = -ball.vel.x;
        }
        if (ball.pos.y > height) {
            return State_Top_Wins;
        }
        Ball::OverlapInfo info;
        if (ball.overlap_paddle(topP, info)) {
            float scale = ball.vel.x * info.normal.x + ball.vel.y * info.normal.y;
            ball.vel.x -= info.normal.x * 2 * scale;
            ball.vel.y -= info.normal.y * 2 * scale;
        }
        if (ball.overlap_paddle(botP, info)) {
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
};

#endif