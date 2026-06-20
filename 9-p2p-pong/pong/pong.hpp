#ifndef APP_PONG_PONG_HPP
#define APP_PONG_PONG_HPP

#include <algorithm>
#include <iostream>
#include "lib/common/common.hpp"
#include "p2p/packet.hpp"
#include "imgui.h"
#include "SDL.h"
#include "paddle.hpp"
#include "ball.hpp"

namespace Pong {
    enum State {
        State_Ongoing,
        State_Right_Wins,
        State_Left_Wins,
    };

    struct Pong {
        float width = 30.0;
        float height = 30.0;
        float pad_h = 3.0;
        float pad_m = 1.0;
        Ball ball;
        Paddle leftP;
        Paddle rightP;

        int leftScore = 0;
        int rightScore = 0;
        bool paddle_update = false;

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
            // Move paddle, projections and ball
            leftP.step(dt, width, height);
            rightP.step(dt, width, height);
            ball.step(dt, width, height);

            // Resolve ball and paddle collision
            auto collide_ball_projection = [&](Ball& ball, Projection& proj) {
                Ball::OverlapInfo info;
                if (!ball.overlap_proj(proj, info)) return;
                Vec2 rel_vel(ball.vel.x - proj.vel.x, ball.vel.y - proj.vel.y);
                float rel_normal_speed = rel_vel.dot(info.normal);
                if (rel_normal_speed >= 0.0f) return;

                ball.vel.x -= info.normal.x * rel_normal_speed;
                ball.vel.y -= info.normal.y * rel_normal_speed;
                proj.vel.x += info.normal.x * rel_normal_speed;
                proj.vel.y += info.normal.y * rel_normal_speed;

                float dx = ball.pos.x - proj.pos.x;
                float dy = ball.pos.y - proj.pos.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                float min_dist = ball.r + proj.r;
                if (dist < min_dist) {
                    float correction = 0.5f * (min_dist - dist + 0.001f);
                    ball.pos.x += info.normal.x * correction;
                    ball.pos.y += info.normal.y * correction;
                    proj.pos.x -= info.normal.x * correction;
                    proj.pos.y -= info.normal.y * correction;
                }
            };
            auto collide_ball_paddle = [&](Ball& ball, Paddle& paddle) {
                Ball::OverlapInfo info;
                if (ball.overlap_paddle(paddle, info) && info.normal.dot(ball.vel) < 0) {
                    float scale = ball.vel.dot(info.normal);
                    ball.vel.x -= info.normal.x * 2 * scale;
                    ball.vel.y -= info.normal.y * 2 * scale;
                }
                for (int i = 0; i < Paddle::N_PROJ; i++) {
                    collide_ball_projection(ball, paddle.projs[i]);
                }
            };
            collide_ball_paddle(ball, leftP);
            collide_ball_paddle(ball, rightP);

            // Check win state
            if (ball.pos.x - ball.r < 0) return State_Right_Wins;
            if (ball.pos.x + ball.r > width) return State_Left_Wins;
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

        void unpack(const P2P::PongConfigBody& config) {
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
                config.left_pos_x,
                config.left_pos_y,
                pad_h
            );
            rightP.reset(
                config.right_pos_x,
                config.right_pos_y,
                pad_h
            );
        }

        P2P::PongConfigBody pack() const {
            return P2P::PongConfigBody{
                .width = width,
                .height = height,
                .pad_h = pad_h,
                .pad_m = pad_m,
                .ball_pos_x = ball.pos.x,
                .ball_pos_y = ball.pos.y,
                .ball_vel_x = ball.vel.x,
                .ball_vel_y = ball.vel.y,
                .ball_r = ball.r,
                .left_pos_x = leftP.pos.x,
                .left_pos_y = leftP.pos.y,
                .left_w = leftP.h,
                .right_pos_x = rightP.pos.x,
                .right_pos_y = rightP.pos.y,
                .right_w = rightP.h
            };
        }

        void process_setup();
        void process_sdl_event(const SDL_Event& event);
        void process_packet(P2P::Packet& packet);
        void process_takedown();

        void draw() {
            process_takedown();

            // Drawing logic
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
            auto draw_proj = [&](const Projection& proj, ImU32 color) {
                if (proj.life < 0) return;
                ImVec2 projPos = world_to_screen(proj.pos.x, proj.pos.y);
                float projRad = std::max(4.0f, ball.r * 0.5f * (scaleX + scaleY));
                drawList->AddCircleFilled(projPos, projRad, color, 24.0f);
            };
            auto draw_paddle = [&](const Paddle& paddle, ImU32 color, float pvx) {
                float depth = 0.7f;
                ImVec2 paddleMin = world_to_screen(paddle.pos.x - depth / 2, paddle.pos.y);
                ImVec2 paddleMax = world_to_screen(paddle.pos.x + depth / 2, paddle.pos.y + paddle.h);
                drawList->AddRectFilled(paddleMin, paddleMax, color, 4.0f);

                Vec2 arrow(pvx, paddle.proj_vy);
                Vec2 paddleCenter(paddle.pos.x + depth / 2, paddle.pos.y + paddle.h / 2);
                arrow.normalise();

                ImVec2 arrowStart = world_to_screen(paddleCenter.x, paddleCenter.y);
                ImVec2 arrowEnd = world_to_screen(paddleCenter.x + arrow.x * 5, paddleCenter.y + arrow.y * 5);
                drawList->AddLine(arrowStart, arrowEnd, color, 1.0f);
                for (int i = 0; i < Paddle::N_PROJ; i++) {
                    draw_proj(paddle.projs[i], color);
                }
            };

            ImGui::InvisibleButton("pong_canvas", canvasSize);
            drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(18, 18, 18, 255), 8.0f);
            drawList->AddRect(boardMin, boardMax, IM_COL32(220, 220, 220, 255), 4.0f, 0, 2.0f);
            ImVec2 centerTop = world_to_screen(width * 0.5f, 0.0f);
            ImVec2 centerBottom = world_to_screen(width * 0.5f, height);
            drawList->AddLine(centerTop, centerBottom, IM_COL32(90, 90, 90, 255), 1.0f);

            draw_paddle(leftP, IM_COL32(104, 211, 145, 255), Paddle::P_VX);
            draw_paddle(rightP, IM_COL32(95, 145, 255, 255), -Paddle::P_VX);

            ImVec2 ballPos = world_to_screen(ball.pos.x, ball.pos.y);
            float ballRadius = std::max(4.0f, ball.r * 0.5f * (scaleX + scaleY));
            drawList->AddCircleFilled(ballPos, ballRadius, IM_COL32(255, 244, 214, 255), 24);
        }
    };

    extern Pong pong;
}

#endif