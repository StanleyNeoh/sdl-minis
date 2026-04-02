#include <gtest/gtest.h>
#include "../vec.hpp"

TEST(PowTest, ZeroPower) {
    EXPECT_FLOAT_EQ((pow<float, 0>(5.0f)), 1.0f);
    EXPECT_FLOAT_EQ((pow<float, 0>(0.0f)), 1.0f);
}

TEST(PowTest, PositivePowers) {
    EXPECT_FLOAT_EQ((pow<float, 1>(3.0f)), 3.0f);
    EXPECT_FLOAT_EQ((pow<float, 2>(3.0f)), 9.0f);
    EXPECT_FLOAT_EQ((pow<float, 3>(2.0f)), 8.0f);
}

TEST(VecTest, DefaultConstruction) {
    Vf2 v{};
    EXPECT_FLOAT_EQ(v[0], 0.0f);
    EXPECT_FLOAT_EQ(v[1], 0.0f);
}

TEST(VecTest, BraceInit) {
    Vf2 v{3.0f, 4.0f};
    EXPECT_FLOAT_EQ(v[0], 3.0f);
    EXPECT_FLOAT_EQ(v[1], 4.0f);
}

TEST(VecTest, Negation) {
    Vf2 v{1.0f, -2.0f};
    Vf2 neg = -v;
    EXPECT_FLOAT_EQ(neg[0], -1.0f);
    EXPECT_FLOAT_EQ(neg[1], 2.0f);
}

TEST(VecTest, Addition) {
    Vf2 a{1.0f, 2.0f};
    Vf2 b{3.0f, 4.0f};
    Vf2 c = a + b;
    EXPECT_FLOAT_EQ(c[0], 4.0f);
    EXPECT_FLOAT_EQ(c[1], 6.0f);
}

TEST(VecTest, Subtraction) {
    Vf2 a{5.0f, 3.0f};
    Vf2 b{2.0f, 1.0f};
    Vf2 c = a - b;
    EXPECT_FLOAT_EQ(c[0], 3.0f);
    EXPECT_FLOAT_EQ(c[1], 2.0f);
}

TEST(VecTest, ElementwiseMultiplication) {
    Vf2 a{2.0f, 3.0f};
    Vf2 b{4.0f, 5.0f};
    Vf2 c = a * b;
    EXPECT_FLOAT_EQ(c[0], 8.0f);
    EXPECT_FLOAT_EQ(c[1], 15.0f);
}

TEST(VecTest, ScalarMultiplicationLeft) {
    Vf2 v{2.0f, 3.0f};
    Vf2 r = 3.0f * v;
    EXPECT_FLOAT_EQ(r[0], 6.0f);
    EXPECT_FLOAT_EQ(r[1], 9.0f);
}

TEST(VecTest, ScalarMultiplicationRight) {
    Vf2 v{2.0f, 3.0f};
    Vf2 r = v * 3.0f;
    EXPECT_FLOAT_EQ(r[0], 6.0f);
    EXPECT_FLOAT_EQ(r[1], 9.0f);
}

TEST(VecTest, PlusEquals) {
    Vf2 a{1.0f, 2.0f};
    Vf2 b{3.0f, 4.0f};
    a += b;
    EXPECT_FLOAT_EQ(a[0], 4.0f);
    EXPECT_FLOAT_EQ(a[1], 6.0f);
}

TEST(VecTest, MinusEquals) {
    Vf2 a{5.0f, 6.0f};
    Vf2 b{1.0f, 2.0f};
    a -= b;
    EXPECT_FLOAT_EQ(a[0], 4.0f);
    EXPECT_FLOAT_EQ(a[1], 4.0f);
}

TEST(VecTest, LenSquared) {
    Vf2 v{3.0f, 4.0f};
    // len<2>() returns sum of squares (squared length)
    EXPECT_FLOAT_EQ((v.len<2>()), 25.0f);
}

TEST(VecTest, LenManhattan) {
    Vf2 v{3.0f, 4.0f};
    // len<1>() returns sum of elements (manhattan-like on raw values)
    EXPECT_FLOAT_EQ((v.len<1>()), 7.0f);
}

TEST(VecTest, DistSquared) {
    Vf2 a{1.0f, 2.0f};
    Vf2 b{4.0f, 6.0f};
    // dist<2>() returns squared distance
    EXPECT_FLOAT_EQ((a.dist<2>(b)), 25.0f);
}

TEST(VecTest, DotProduct) {
    Vf2 a{1.0f, 2.0f};
    Vf2 b{3.0f, 4.0f};
    // dot = 1*3 + 2*4 = 11
    EXPECT_FLOAT_EQ(a.dot(b), 11.0f);
}

TEST(VecTest, DotProductPerpendicular) {
    Vf2 a{1.0f, 0.0f};
    Vf2 b{0.0f, 1.0f};
    EXPECT_FLOAT_EQ(a.dot(b), 0.0f);
}

TEST(VecTest, ThreeDimensional) {
    Vec<float, 3> a{{1.0f, 2.0f, 3.0f}};
    Vec<float, 3> b{{4.0f, 5.0f, 6.0f}};
    auto c = a + b;
    EXPECT_FLOAT_EQ(c[0], 5.0f);
    EXPECT_FLOAT_EQ(c[1], 7.0f);
    EXPECT_FLOAT_EQ(c[2], 9.0f);
}

TEST(VecTest, StreamOutput) {
    Vf2 v{1.5f, 2.5f};
    std::ostringstream os;
    os << v;
    EXPECT_EQ(os.str(), "(1.5,2.5)");
}
