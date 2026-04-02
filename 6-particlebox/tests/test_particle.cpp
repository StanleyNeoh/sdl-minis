#include <gtest/gtest.h>
#include "../particle.hpp"

TEST(ParticleTest, Construction) {
    Pf2 p(Vf2{1.0f, 2.0f}, Vf2{3.0f, 4.0f}, 5.0f);
    EXPECT_FLOAT_EQ(p.pos[0], 1.0f);
    EXPECT_FLOAT_EQ(p.pos[1], 2.0f);
    EXPECT_FLOAT_EQ(p.vel[0], 3.0f);
    EXPECT_FLOAT_EQ(p.vel[1], 4.0f);
    EXPECT_FLOAT_EQ(p.rad, 5.0f);
}

TEST(ParticleTest, Step) {
    Pf2 p(Vf2{0.0f, 0.0f}, Vf2{1.0f, 2.0f}, 1.0f);
    p.step();
    EXPECT_FLOAT_EQ(p.pos[0], 1.0f);
    EXPECT_FLOAT_EQ(p.pos[1], 2.0f);
    // velocity unchanged
    EXPECT_FLOAT_EQ(p.vel[0], 1.0f);
    EXPECT_FLOAT_EQ(p.vel[1], 2.0f);
}

TEST(ParticleTest, MultipleSteps) {
    Pf2 p(Vf2{0.0f, 0.0f}, Vf2{0.5f, -0.5f}, 1.0f);
    p.step();
    p.step();
    p.step();
    EXPECT_FLOAT_EQ(p.pos[0], 1.5f);
    EXPECT_FLOAT_EQ(p.pos[1], -1.5f);
}

TEST(ParticleTest, IsOverlapTrue) {
    // Two particles at distance 1, combined radii = 2 → overlapping
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{0.0f, 0.0f}, 1.0f);
    Pf2 b(Vf2{1.0f, 0.0f}, Vf2{0.0f, 0.0f}, 1.0f);
    // dist^2 = 1, (r1+r2)^2 = 4, 1 < 4 → overlap
    EXPECT_TRUE(a.is_overlap(b));
}

TEST(ParticleTest, IsOverlapFalse) {
    // Two particles at distance 5, combined radii = 2 → not overlapping
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{0.0f, 0.0f}, 1.0f);
    Pf2 b(Vf2{3.0f, 4.0f}, Vf2{0.0f, 0.0f}, 1.0f);
    // dist^2 = 25, (r1+r2)^2 = 4, 25 >= 4 → no overlap
    EXPECT_FALSE(a.is_overlap(b));
}

TEST(ParticleTest, IsOverlapBarelyTouching) {
    // Exactly touching: distance = sum of radii
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{0.0f, 0.0f}, 1.0f);
    Pf2 b(Vf2{2.0f, 0.0f}, Vf2{0.0f, 0.0f}, 1.0f);
    // dist^2 = 4, (r1+r2)^2 = 4, not strictly less → no overlap
    EXPECT_FALSE(a.is_overlap(b));
}

TEST(ParticleTest, IsApproachingTrue) {
    // b is to the right, a moves right, b is stationary → approaching
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{1.0f, 0.0f}, 1.0f);
    Pf2 b(Vf2{3.0f, 0.0f}, Vf2{0.0f, 0.0f}, 1.0f);
    EXPECT_TRUE(a.is_approaching(b));
}

TEST(ParticleTest, IsApproachingFalse) {
    // Both moving apart
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{-1.0f, 0.0f}, 1.0f);
    Pf2 b(Vf2{3.0f, 0.0f}, Vf2{1.0f, 0.0f}, 1.0f);
    EXPECT_FALSE(a.is_approaching(b));
}

TEST(ParticleTest, IsCollidingTrue) {
    // Overlapping and approaching
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{1.0f, 0.0f}, 1.0f);
    Pf2 b(Vf2{1.0f, 0.0f}, Vf2{0.0f, 0.0f}, 1.0f);
    EXPECT_TRUE(a.is_colliding(b));
}

TEST(ParticleTest, IsCollidingFalseNotOverlapping) {
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{1.0f, 0.0f}, 0.1f);
    Pf2 b(Vf2{5.0f, 0.0f}, Vf2{0.0f, 0.0f}, 0.1f);
    EXPECT_FALSE(a.is_colliding(b));
}

TEST(ParticleTest, IsCollidingFalseNotApproaching) {
    // Overlapping but moving apart
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{-1.0f, 0.0f}, 1.0f);
    Pf2 b(Vf2{1.0f, 0.0f}, Vf2{1.0f, 0.0f}, 1.0f);
    EXPECT_FALSE(a.is_colliding(b));
}

TEST(ParticleTest, ResolveCollisionHeadOn) {
    // Head-on 1D collision: equal mass particles swap velocities
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{1.0f, 0.0f}, 1.0f);
    Pf2 b(Vf2{1.5f, 0.0f}, Vf2{-1.0f, 0.0f}, 1.0f);
    resolve_collision(a, b);
    EXPECT_FLOAT_EQ(a.vel[0], -1.0f);
    EXPECT_FLOAT_EQ(b.vel[0], 1.0f);
    // y velocities unchanged
    EXPECT_FLOAT_EQ(a.vel[1], 0.0f);
    EXPECT_FLOAT_EQ(b.vel[1], 0.0f);
}

TEST(ParticleTest, ResolveCollisionStationaryTarget) {
    // One particle hits a stationary one head-on along x
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{2.0f, 0.0f}, 1.0f);
    Pf2 b(Vf2{1.5f, 0.0f}, Vf2{0.0f, 0.0f}, 1.0f);
    resolve_collision(a, b);
    EXPECT_FLOAT_EQ(a.vel[0], 0.0f);
    EXPECT_FLOAT_EQ(b.vel[0], 2.0f);
}

TEST(ParticleTest, ResolveCollisionMomentumConserved) {
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{3.0f, 1.0f}, 1.0f);
    Pf2 b(Vf2{1.5f, 0.0f}, Vf2{-1.0f, 2.0f}, 1.0f);
    Vf2 total_before = a.vel + b.vel;
    resolve_collision(a, b);
    Vf2 total_after = a.vel + b.vel;
    EXPECT_FLOAT_EQ(total_after[0], total_before[0]);
    EXPECT_FLOAT_EQ(total_after[1], total_before[1]);
}

TEST(ParticleTest, ResolveCollisionKineticEnergyConserved) {
    Pf2 a(Vf2{0.0f, 0.0f}, Vf2{3.0f, 1.0f}, 1.0f);
    Pf2 b(Vf2{1.5f, 0.0f}, Vf2{-1.0f, 2.0f}, 1.0f);
    float ke_before = a.vel.template len<2>() + b.vel.template len<2>();
    resolve_collision(a, b);
    float ke_after = a.vel.template len<2>() + b.vel.template len<2>();
    EXPECT_NEAR(ke_after, ke_before, 1e-5f);
}

TEST(ParticleTest, StreamOutput) {
    Pf2 p(Vf2{1.0f, 2.0f}, Vf2{3.0f, 4.0f}, 5.0f);
    std::ostringstream os;
    os << p;
    EXPECT_EQ(os.str(), "[p: (1,2), v: (3,4), r: 5]");
}
