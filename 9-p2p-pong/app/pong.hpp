#ifndef PONG_HPP
#define PONG_HPP

#include <algorithm>
#include <iostream>
#include "lib/common/utils.hpp"
#include "p2p/packet.hpp"
#include "imgui.h"

struct Pong {
    enum State {
        State_Ongoing,
        State_Right_Wins,
        State_Left_Wins,
    };

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
                float _proj_ay = abs(proj_ay) > 1e-6
                    ? proj_ay
                    : proj_vy > 0 
                    ? -Paddle::P_AY
                    : proj_vy < 0
                    ? Paddle::P_AY
                    : 0;
                proj_vy = proj_vy + _proj_ay * dt;
                if (proj_vy < -Paddle::P_VY) proj_vy = -Paddle::P_VY;
                else if (proj_vy > Paddle::P_VY) proj_vy = Paddle::P_VY;
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
            proj_ay = padbody.p_acc_y;
            proj_vy = padbody.p_vel_y;
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

    float width;
    float height;
    float pad_h;
    float pad_m;
    Ball ball;
    Paddle leftP;
    Paddle rightP;
    int clientScore = 0;
    int masterScore = 0;

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
        if (ball.pos.x - ball.r < 0) return State_Left_Wins;
        if (ball.pos.x + ball.r > width) return State_Right_Wins;
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
        auto draw_proj = [&](const Pong::Projection& proj, ImU32 color) {
            if (proj.life < 0) return;
            ImVec2 projPos = world_to_screen(proj.pos.x, proj.pos.y);
            float projRad = std::max(4.0f, ball.r * 0.5f * (scaleX + scaleY));
            drawList->AddCircleFilled(projPos, projRad, color, 24.0f);
        };
        auto draw_paddle = [&](const Pong::Paddle& paddle, ImU32 color, float pvx) {
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

#endif