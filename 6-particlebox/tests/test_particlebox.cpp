#include <gtest/gtest.h>
#include "../particle.hpp"

using namespace Entity;

TEST(ParticleBoxTest, AddParticle) {
    ParticleBox box;
    EXPECT_EQ(box.particles.size(), 0);
    box.add_particle(Particle(0.0f, 0.0f, 1.0f, 0.0f, 1.0f));
    EXPECT_EQ(box.particles.size(), 1);
    box.add_particle(Particle(5.0f, 0.0f, -1.0f, 0.0f, 1.0f));
    EXPECT_EQ(box.particles.size(), 2);
}

TEST(ParticleBoxTest, StepMovesParticles) {
    ParticleBox box;
    box.add_particle(Particle(0.0f, 0.0f, 1.0f, 2.0f, 1.0f));
    box.add_particle(Particle(10.0f, 10.0f, -0.5f, 0.5f, 1.0f));
    box.step();
    EXPECT_FLOAT_EQ(box.particles[0].x, 1.0f);
    EXPECT_FLOAT_EQ(box.particles[0].y, 2.0f);
    EXPECT_FLOAT_EQ(box.particles[1].x, 9.5f);
    EXPECT_FLOAT_EQ(box.particles[1].y, 10.5f);
}

TEST(ParticleBoxTest, StepNoCollision) {
    // Particles far apart — velocities unchanged after step
    ParticleBox box;
    box.add_particle(Particle(0.0f, 0.0f, 1.0f, 0.0f, 1.0f));
    box.add_particle(Particle(100.0f, 0.0f, -1.0f, 0.0f, 1.0f));
    box.step();
    EXPECT_FLOAT_EQ(box.particles[0].vx, 1.0f);
    EXPECT_FLOAT_EQ(box.particles[0].vy, 0.0f);
    EXPECT_FLOAT_EQ(box.particles[1].vx, -1.0f);
    EXPECT_FLOAT_EQ(box.particles[1].vy, 0.0f);
}

TEST(ParticleBoxTest, StepResolvesHeadOnCollision) {
    // After step positions will be (0.5,0) and (2.0,0) — overlapping and approaching
    ParticleBox box;
    box.add_particle(Particle(0.0f, 0.0f, 0.5f, 0.0f, 1.0f));
    box.add_particle(Particle(2.5f, 0.0f, -0.5f, 0.0f, 1.0f));
    box.step();
    // Velocities swap after collision resolution
    EXPECT_FLOAT_EQ(box.particles[0].vx, -0.5f);
    EXPECT_FLOAT_EQ(box.particles[1].vx, 0.5f);
}

TEST(ParticleBoxTest, StepMultipleSteps) {
    // After collision, subsequent steps should not re-collide (particles separate)
    ParticleBox box;
    box.add_particle(Particle(0.0f, 0.0f, 0.5f, 0.0f, 1.0f));
    box.add_particle(Particle(2.5f, 0.0f, -0.5f, 0.0f, 1.0f));

    box.step(); // collide and resolve
    float v0 = box.particles[0].vx;
    float v1 = box.particles[1].vx;

    box.step(); // should just move, no further collision
    EXPECT_FLOAT_EQ(box.particles[0].vx, v0);
    EXPECT_FLOAT_EQ(box.particles[1].vx, v1);
}

TEST(ParticleBoxTest, StepSingleParticle) {
    ParticleBox box;
    box.add_particle(Particle(1.0f, 2.0f, 3.0f, 4.0f, 1.0f));
    box.step();
    EXPECT_FLOAT_EQ(box.particles[0].x, 4.0f);
    EXPECT_FLOAT_EQ(box.particles[0].y, 6.0f);
    EXPECT_FLOAT_EQ(box.particles[0].vx, 3.0f);
    EXPECT_FLOAT_EQ(box.particles[0].vy, 4.0f);
}

TEST(ParticleBoxTest, StepEmpty) {
    ParticleBox box;
    box.step(); // should not crash
    EXPECT_EQ(box.particles.size(), 0);
}

TEST(ParticleBoxTest, ThreeParticlesOneCollision) {
    // Only the close pair collides; the third is far away
    ParticleBox box;
    box.add_particle(Particle(0.0f, 0.0f, 0.5f, 0.0f, 1.0f));
    box.add_particle(Particle(2.5f, 0.0f, -0.5f, 0.0f, 1.0f));
    box.add_particle(Particle(50.0f, 50.0f, 0.0f, 0.0f, 1.0f));
    box.step();
    // First two swap velocities
    EXPECT_FLOAT_EQ(box.particles[0].vx, -0.5f);
    EXPECT_FLOAT_EQ(box.particles[1].vx, 0.5f);
    // Third unchanged
    EXPECT_FLOAT_EQ(box.particles[2].vx, 0.0f);
    EXPECT_FLOAT_EQ(box.particles[2].vy, 0.0f);
}
