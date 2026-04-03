#include <gtest/gtest.h>
#include "../particle.hpp"

using namespace Entity;

TEST(ParticleTest, Construction) {
    Particle p(1.0f, 2.0f, 3.0f, 4.0f, 5.0f);
    EXPECT_FLOAT_EQ(p.x, 1.0f);
    EXPECT_FLOAT_EQ(p.y, 2.0f);
    EXPECT_FLOAT_EQ(p.vx, 3.0f);
    EXPECT_FLOAT_EQ(p.vy, 4.0f);
    EXPECT_FLOAT_EQ(p.r, 5.0f);
}

TEST(ParticleTest, Step) {
    Particle p(0.0f, 0.0f, 1.0f, 2.0f, 1.0f);
    p.step();
    EXPECT_FLOAT_EQ(p.x, 1.0f);
    EXPECT_FLOAT_EQ(p.y, 2.0f);
    // velocity unchanged
    EXPECT_FLOAT_EQ(p.vx, 1.0f);
    EXPECT_FLOAT_EQ(p.vy, 2.0f);
}

TEST(ParticleTest, MultipleSteps) {
    Particle p(0.0f, 0.0f, 0.5f, -0.5f, 1.0f);
    p.step();
    p.step();
    p.step();
    EXPECT_FLOAT_EQ(p.x, 1.5f);
    EXPECT_FLOAT_EQ(p.y, -1.5f);
}

TEST(ParticleTest, IsOverlapTrue) {
    // Two particles at distance 1, combined rii = 2 → overlapping
    Particle a(0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    Particle b(1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    // dist^2 = 1, (r1+r2)^2 = 4, 1 < 4 → overlap
    EXPECT_TRUE(a.is_overlap(b));
}

TEST(ParticleTest, IsOverlapFalse) {
    // Two particles at distance 5, combined rii = 2 → not overlapping
    Particle a(0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    Particle b(3.0f, 4.0f, 0.0f, 0.0f, 1.0f);
    // dist^2 = 25, (r1+r2)^2 = 4, 25 >= 4 → no overlap
    EXPECT_FALSE(a.is_overlap(b));
}

TEST(ParticleTest, IsOverlapBarelyTouching) {
    // Exactly touching: distance = sum of rii
    Particle a(0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    Particle b(2.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    // dist^2 = 4, (r1+r2)^2 = 4, not strictly less → no overlap
    EXPECT_FALSE(a.is_overlap(b));
}

TEST(ParticleTest, IsApproachingTrue) {
    // b is to the right, a moves right, b is stationary → approaching
    Particle a(0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    Particle b(3.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_TRUE(a.is_approaching(b));
}

TEST(ParticleTest, IsApproachingFalse) {
    // Both moving apart
    Particle a(0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    Particle b(3.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    EXPECT_FALSE(a.is_approaching(b));
}

TEST(ParticleTest, IsCollidingTrue) {
    // Overlapping and approaching
    Particle a(0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    Particle b(1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_TRUE(a.is_colliding(b));
}

TEST(ParticleTest, IsCollidingFalseNotOverlapping) {
    Particle a(0.0f, 0.0f, 1.0f, 0.0f, 0.1f);
    Particle b(5.0f, 0.0f, 0.0f, 0.0f, 0.1f);
    EXPECT_FALSE(a.is_colliding(b));
}

TEST(ParticleTest, IsCollidingFalseNotApproaching) {
    // Overlapping but moving apart
    Particle a(0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    Particle b(1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    EXPECT_FALSE(a.is_colliding(b));
}

TEST(ParticleTest, ResolveCollisionHeadOn) {
    // Head-on 1D collision: equal mass particles swap velocities
    Particle a(0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    Particle b(1.5f, 0.0f, -1.0f, 0.0f, 1.0f);
    resolve_collision(a, b);
    EXPECT_FLOAT_EQ(a.vx, -1.0f);
    EXPECT_FLOAT_EQ(b.vx, 1.0f);
    // y velocities unchanged
    EXPECT_FLOAT_EQ(a.vy, 0.0f);
    EXPECT_FLOAT_EQ(b.vy, 0.0f);
}

TEST(ParticleTest, ResolveCollisionStationaryTarget) {
    // One particle hits a stationary one head-on along x
    Particle a(0.0f, 0.0f, 2.0f, 0.0f, 1.0f);
    Particle b(1.5f, 0.0f, 0.0f, 0.0f, 1.0f);
    resolve_collision(a, b);
    EXPECT_FLOAT_EQ(a.vx, 0.0f);
    EXPECT_FLOAT_EQ(b.vx, 2.0f);
}

TEST(ParticleTest, ResolveCollisionMomentumConserved) {
    Particle a(0.0f, 0.0f, 3.0f, 1.0f, 1.0f);
    Particle b(1.5f, 0.0f, -1.0f, 2.0f, 1.0f);
    float tvx_before = a.vx + b.vx;
    float tvy_before = a.vy + b.vy;
    resolve_collision(a, b);
    float tvx_after = a.vx + b.vx;
    float tvy_after = a.vy + b.vy;
    EXPECT_FLOAT_EQ(tvx_before, tvx_after);
    EXPECT_FLOAT_EQ(tvy_before, tvy_after);
}

TEST(ParticleTest, ResolveCollisionKineticEnergyConserved) {
    Particle a(0.0f, 0.0f, 3.0f, 1.0f, 1.0f);
    Particle b(1.5f, 0.0f, -1.0f, 2.0f, 1.0f);
    float ke_before = (a.vx * a.vx) + (a.vy + a.vy) + (b.vx * b.vx) + (b.vy * b.vy);
    resolve_collision(a, b);
    float ke_after = (a.vx * a.vx) + (a.vy + a.vy) + (b.vx * b.vx) + (b.vy * b.vy);
    EXPECT_NEAR(ke_after, ke_before, 1e-5f);
}

TEST(ParticleTest, StreamOutput) {
    Particle p(1.0f, 2.0f, 3.0f, 4.0f, 5.0f);
    std::ostringstream os;
    os << p;
    EXPECT_EQ(os.str(), "[p: (1,2), v: (3,4), r: 5]");
}
