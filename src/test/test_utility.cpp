//
// Created by lby on 10/13/23.
//

#include "gtest/gtest.h"
#include "../dsas/utility.h"
using namespace util;
TEST(UtilityTest, TestCrossProduct) {
    std::vector<double> vec1{0.0, 1.0};
    std::vector<double> vec2{1.0, 0.0};

    auto ans = util::crossProduct<double>(vec1, vec2);

    EXPECT_EQ(ans, -1);
}

TEST(UtilityTest, TestIntersect){
    using namespace gm;
    Point<double> p1 {0, 0};
    Point<double> p2 {1, 1};
    Point<double> p3 {0, 1};
    Point<double> p4 {1, 0};

    ASSERT_TRUE(util::isTwoSegmentIntersected<double>(p1, p2, p3, p4));
}

TEST(UtilityTest, TestIntersectPoint){
    using namespace gm;
    Point<double> p1 {0, 0};
    Point<double> p2 {1, 1};
    Point<double> p3 {0, 1};
    Point<double> p4 {1, 0};

    auto intersection = util::computeIntersectPoint<double>(p1, p2, p3, p4);

    EXPECT_EQ(intersection.x, 0.5);
    EXPECT_EQ(intersection.y, 0.5);
}