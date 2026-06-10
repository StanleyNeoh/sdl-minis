#ifndef PONG_HPP
#define PONG_HPP

#include <algorithm>
#include <iostream>
#include "utils.hpp"
#include "packet/packet.hpp"
#include "imgui.h"

struct Pong {
    enum State {
        State_Ongoing,
        State_Right_Wins,
        State_Left_Wins,
    };

    struct Paddle {
        Vec2 pos;
        float vy = 0;
        float h;

        Paddle(float x, float y, float h): pos(x, y), vy(0), h(h) {}

        void move(float vy) {
            this->vy = vy;
        }

        void reset(float x, float y, float _h) {
            pos.x = x;
            pos.y = y;
            h = _h;
        }

        friend std::ostream& operator<<(std::ostream& o, const Paddle& paddle) {
            o << "Paddle{pos=" << paddle.pos << ",h=" << paddle.h << "}";
            return o;
        }

        void unpack(const Packet::PongPaddleBody& padbody) {
            pos.y = padbody.pos_y;
            vy = padbody.vel_y;
        }

        Packet::PongPaddleBody pack() const {
            return Packet::PongPaddleBody{
                .pos_y = pos.y,
                .vel_y = vy
            };
        }
    };

    struct Ball {
        Vec2 pos;
        Vec2 vel;
        float r;
        Ball(float x, float y, float r = 1.0, float v = 10.0): pos(x, y), vel(Vec2::rand(v)), r(r) {}

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

        void reset(float x, float y, float v = 10.0) {
            pos.x = x;
            pos.y = y;
            vel = Vec2::rand(v);
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
    float pad_h;
    float pad_m;
    Ball ball;
    Paddle leftP;
    Paddle rightP;

    Pong(float width = 30.0, float height = 30.0, float ball_r = 1.0, float pad_h = 3.0, float pad_m = 1.0): 
        width(width), 
        height(height),
        pad_h(pad_h),
        pad_m(pad_m),
        ball(width / 2, height / 2, ball_r),
        leftP(pad_m, height / 2 - pad_h / 2, pad_h),
        rightP(width - pad_m, height / 2 - pad_h / 2, pad_h)
    {}

    void reset() {
        ball.reset(width / 2, height / 2);
        leftP.reset(pad_m, height / 2 - pad_h / 2, pad_h),
        rightP.reset(width - pad_m, height / 2 - pad_h / 2, pad_h);
    }

    State step(float dt) {
        ball.pos.x = ball.pos.x + ball.vel.x * dt;
        ball.pos.y = ball.pos.y + ball.vel.y * dt;
        leftP.pos.y = SDL_clamp(leftP.pos.y + leftP.vy * dt, 0, width - leftP.h);
        rightP.pos.y = SDL_clamp(rightP.pos.y + rightP.vy * dt, 0, width - rightP.h);
        if (ball.pos.y - ball.r < 0) {
            ball.pos.y = ball.r;
            if (ball.vel.y < 0) ball.vel.y = -ball.vel.y;
        }
        if (ball.pos.x - ball.r < 0) {
            return State_Left_Wins;
        }
        if (ball.pos.y + ball.r > height) {
            ball.pos.y = height - ball.r;
            if (ball.vel.y > 0) ball.vel.y = -ball.vel.y;
        }
        if (ball.pos.x + ball.r > width) {
            return State_Right_Wins;
        }
        Ball::OverlapInfo info;
        if (ball.overlap_paddle(leftP, info) && info.normal.dot(ball.vel) < 0) {
            float scale = ball.vel.x * info.normal.x + ball.vel.y * info.normal.y;
            ball.vel.x -= info.normal.x * 2 * scale;
            ball.vel.y -= info.normal.y * 2 * scale;
        }
        if (ball.overlap_paddle(rightP, info) && info.normal.dot(ball.vel) < 0) {
            float scale = ball.vel.x * info.normal.x + ball.vel.y * info.normal.y;
            ball.vel.x -= info.normal.x * 2 * scale;
            ball.vel.y -= info.normal.y * 2 * scale;
        }
        return State_Ongoing;
    }

    friend std::ostream& operator<<(std::ostream& o, const Pong& pong) {
        o << "Pong{\n" 
            << "  Left" << pong.leftP << "\n"
            << "  Right" << pong.rightP << "\n"
            << "  " << pong.ball << "\n"
            << "}";
        return o;
    }

    void unpack(const Packet::PongConfigBody& config) {
        width = config.width;
        height = config.height;
        pad_h = config.pad_h;
        pad_m = config.pad_m;
        ball.reset(
            config.ball_pos_x, 
            config.ball_pos_y, 
            config.ball_vel_x, 
            config.ball_vel_y
        );
        ball.r = config.ball_r;
        leftP.reset(
            config.top_pos_x,
            config.top_pos_y,
            pad_h
        );
        rightP.reset(
            config.bot_pos_x,
            config.bot_pos_y,
            pad_h
        );
    }

    Packet::PongConfigBody pack() const {
        return Packet::PongConfigBody{
            .width = width,
            .height = height,
            .pad_h = pad_h,
            .pad_m = pad_m,
            .ball_pos_x = ball.pos.x,
            .ball_pos_y = ball.pos.y,
            .ball_vel_x = ball.vel.x,
            .ball_vel_y = ball.vel.y,
            .ball_r = ball.r,
            .top_pos_x = leftP.pos.x,
            .top_pos_y = leftP.pos.y,
            .top_w = leftP.h,
            .bot_pos_x = rightP.pos.x,
            .bot_pos_y = rightP.pos.y,
            .bot_w = rightP.h
        };
    }

    void draw() {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float canvasSide = std::min(avail.x, avail.y);

        ImVec2 canvasSize(canvasSide, canvasSide);
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 margin((avail.x - canvasSide) / 2, (avail.y - canvasSide) / 2);
        ImVec2 boardMin(canvasPos.x, canvasPos.y);
        ImVec2 boardMax(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
        float scaleX = (boardMax.x - boardMin.x) / width;
        float scaleY = (boardMax.y - boardMin.y) / height;
        auto world_to_screen = [&](float x, float y) {
            return ImVec2(boardMin.x + x * scaleX, boardMin.y + y * scaleY);
        };
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        auto draw_paddle = [&](const Pong::Paddle& paddle, ImU32 color) {
            float depth = 0.7f;
            ImVec2 paddleMin = world_to_screen(paddle.pos.x - depth / 2, paddle.pos.y);
            ImVec2 paddleMax = world_to_screen(paddle.pos.x + depth / 2, paddle.pos.y + paddle.h);
            drawList->AddRectFilled(paddleMin, paddleMax, color, 4.0f);
        };

        ImGui::InvisibleButton("pong_canvas", canvasSize);
        drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(18, 18, 18, 255), 8.0f);
        drawList->AddRect(boardMin, boardMax, IM_COL32(220, 220, 220, 255), 4.0f, 0, 2.0f);
        ImVec2 centerTop = world_to_screen(width * 0.5f, 0.0f);
        ImVec2 centerBottom = world_to_screen(width * 0.5f, height);
        drawList->AddLine(centerTop, centerBottom, IM_COL32(90, 90, 90, 255), 1.0f);

        draw_paddle(leftP, IM_COL32(104, 211, 145, 255));
        draw_paddle(rightP, IM_COL32(95, 145, 255, 255));

        ImVec2 ballPos = world_to_screen(ball.pos.x, ball.pos.y);
        float ballRadius = std::max(4.0f, ball.r * 0.5f * (scaleX + scaleY));
        drawList->AddCircleFilled(ballPos, ballRadius, IM_COL32(255, 244, 214, 255), 24);
    }
};

#endif