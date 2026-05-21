#ifndef GAME_HPP
#define GAME_HPP

#include <random>
#include <cmath>

namespace Game {
    namespace Rand {
        static std::mt19937 engine(std::random_device{}());

        int integer(int max, int min = 0) {
            std::uniform_int_distribution<int> distribution(min, max);
            return distribution(engine);
        }

        float real(float max = 1, float min = 0) {
            std::uniform_real_distribution<float> distribution(min, max);
            return distribution(engine);
        }
    };

    enum State {
        State_Ongoing,
        State_Top_Wins,
        State_Bot_Wins,
    };

    struct Vec2 {
        float x = 0;
        float y = 0;

        static Vec2 rand_unit() {
            float l = Rand::real();
            float d = Rand::real(2 * M_PI, 0);
            return Vec2(l * std::sin(d), l * std::cos(d));
        }

        Vec2() = default;
        Vec2(float x, float y): x(x), y(y) {}

        float l2() const {
            return x * x + y * y;
        }

        float l() const {
            return std::sqrt(l2());
        }

        void normalise() {
            float len = l();
            x /= len;
            y /= len;
        }

        Vec2 unit() const {
            Vec2 v(x, y);
            v.normalise();
            return v;
        }
    };

    struct Paddle {
        float x;
        float y;
        float w;

        Paddle(float x, float y, float w): x(x), y(y), w(w) {}

        void move(float dx) {
            x += dx;
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
                overlap_pt(pad.x, pad.y, info) 
                || overlap_pt(pad.x + pad.w, pad.y, info)
                || overlap_hsec(pad.x, pad.y, pad.w, info)
            );
        }
    };

    struct Game {
        float width;
        float height;
        Ball ball;
        Paddle topP;
        Paddle botP;

        Game(float width = 30.0, float height = 30.0, float ball_r = 1.0, float pad_w = 3.0, float pad_m = 1.0): 
            width(width), 
            height(height),
            ball(width / 2, height / 2),
            topP(width - pad_w / 2, pad_m, pad_w),
            botP(width - pad_w / 2, height - pad_m, pad_w)
        {}

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
    };
}

#endif