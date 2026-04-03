#include <gtest/gtest.h>
#include "../particlebox.hpp"

TEST(ParticleBoxTest, AddParticle) {
    ParticleBox box;
    EXPECT_EQ(box.particles.size(), 0);
    box.add_particle(Pf2(Vf2{0.0f, 0.0f}, Vf2{1.0f, 0.0f}, 1.0f));
    EXPECT_EQ(box.particles.size(), 1);
    box.add_particle(Pf2(Vf2{5.0f, 0.0f}, Vf2{-1.0f, 0.0f}, 1.0f));
    EXPECT_EQ(box.particles.size(), 2);
}

TEST(ParticleBoxTest, StepMovesParticles) {
    ParticleBox box;
    box.add_particle(Pf2(Vf2{0.0f, 0.0f}, Vf2{1.0f, 2.0f}, 1.0f));
    box.add_particle(Pf2(Vf2{10.0f, 10.0f}, Vf2{-0.5f, 0.5f}, 1.0f));
    box.step();
    EXPECT_FLOAT_EQ(box.particles[0].pos[0], 1.0f);
    EXPECT_FLOAT_EQ(box.particles[0].pos[1], 2.0f);
    EXPECT_FLOAT_EQ(box.particles[1].pos[0], 9.5f);
    EXPECT_FLOAT_EQ(box.particles[1].pos[1], 10.5f);
}

TEST(ParticleBoxTest, StepNoCollision) {
    // Particles far apart — velocities unchanged after step
    ParticleBox box;
    box.add_particle(Pf2(Vf2{0.0f, 0.0f}, Vf2{1.0f, 0.0f}, 1.0f));
    box.add_particle(Pf2(Vf2{100.0f, 0.0f}, Vf2{-1.0f, 0.0f}, 1.0f));
    box.step();
    EXPECT_FLOAT_EQ(box.particles[0].vel[0], 1.0f);
    EXPECT_FLOAT_EQ(box.particles[0].vel[1], 0.0f);
    EXPECT_FLOAT_EQ(box.particles[1].vel[0], -1.0f);
    EXPECT_FLOAT_EQ(box.particles[1].vel[1], 0.0f);
}

TEST(ParticleBoxTest, StepResolvesHeadOnCollision) {
    // After step positions will be (0.5,0) and (2.0,0) — overlapping and approaching
    ParticleBox box;
    box.add_particle(Pf2(Vf2{0.0f, 0.0f}, Vf2{0.5f, 0.0f}, 1.0f));
    box.add_particle(Pf2(Vf2{2.5f, 0.0f}, Vf2{-0.5f, 0.0f}, 1.0f));
    box.step();
    // Velocities swap after collision resolution
    EXPECT_FLOAT_EQ(box.particles[0].vel[0], -0.5f);
    EXPECT_FLOAT_EQ(box.particles[1].vel[0], 0.5f);
}

TEST(ParticleBoxTest, StepMultipleSteps) {
    // After collision, subsequent steps should not re-collide (particles separate)
    ParticleBox box;
    box.add_particle(Pf2(Vf2{0.0f, 0.0f}, Vf2{0.5f, 0.0f}, 1.0f));
    box.add_particle(Pf2(Vf2{2.5f, 0.0f}, Vf2{-0.5f, 0.0f}, 1.0f));

    box.step(); // collide and resolve
    float v0 = box.particles[0].vel[0];
    float v1 = box.particles[1].vel[0];

    box.step(); // should just move, no further collision
    EXPECT_FLOAT_EQ(box.particles[0].vel[0], v0);
    EXPECT_FLOAT_EQ(box.particles[1].vel[0], v1);
}

TEST(ParticleBoxTest, StepSingleParticle) {
    ParticleBox box;
    box.add_particle(Pf2(Vf2{1.0f, 2.0f}, Vf2{3.0f, 4.0f}, 1.0f));
    box.step();
    EXPECT_FLOAT_EQ(box.particles[0].pos[0], 4.0f);
    EXPECT_FLOAT_EQ(box.particles[0].pos[1], 6.0f);
    EXPECT_FLOAT_EQ(box.particles[0].vel[0], 3.0f);
    EXPECT_FLOAT_EQ(box.particles[0].vel[1], 4.0f);
}

TEST(ParticleBoxTest, StepEmpty) {
    ParticleBox box;
    box.step(); // should not crash
    EXPECT_EQ(box.particles.size(), 0);
}

TEST(ParticleBoxTest, ThreeParticlesOneCollision) {
    // Only the close pair collides; the third is far away
    ParticleBox box;
    box.add_particle(Pf2(Vf2{0.0f, 0.0f}, Vf2{0.5f, 0.0f}, 1.0f));
    box.add_particle(Pf2(Vf2{2.5f, 0.0f}, Vf2{-0.5f, 0.0f}, 1.0f));
    box.add_particle(Pf2(Vf2{50.0f, 50.0f}, Vf2{0.0f, 0.0f}, 1.0f));
    box.step();
    // First two swap velocities
    EXPECT_FLOAT_EQ(box.particles[0].vel[0], -0.5f);
    EXPECT_FLOAT_EQ(box.particles[1].vel[0], 0.5f);
    // Third unchanged
    EXPECT_FLOAT_EQ(box.particles[2].vel[0], 0.0f);
    EXPECT_FLOAT_EQ(box.particles[2].vel[1], 0.0f);
}
