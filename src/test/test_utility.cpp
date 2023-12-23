//
// Created by lby on 10/13/23.
//

#include "../dsas/utility.h"
#include "gtest/gtest.h"
#define TOL 1e-4
using namespace util;
TEST(UtilityTest, TestCrossProduct) {
  std::vector<double> vec1{0.0, 1.0};
  std::vector<double> vec2{1.0, 0.0};

  auto ans = util::crossProduct<double>(vec1, vec2);

  EXPECT_EQ(ans, -1);
}

TEST(UtilityTest, TestIntersect) {
  using namespace gm;
  {
    Point<double> p1{0, 0};
    Point<double> p2{1, 1};
    Point<double> p3{0, 1};
    Point<double> p4{1, 0};

    ASSERT_TRUE(util::isTwoSegmentIntersected<double>(p1, p2, p3, p4));
  }
  {
    Point<double> p1{0, 0};
    Point<double> p2{2, 0};
    Point<double> p3{1, -1};
    Point<double> p4{1, 1};

    ASSERT_TRUE(util::isTwoSegmentIntersected<double>(p1, p2, p3, p4));
  }
}

TEST(UtilityTest, TestIntersectPoint) {
  using namespace gm;
  Point<double> p1{0, 0};
  Point<double> p2{1, 1};
  Point<double> p3{0, 1};
  Point<double> p4{1, 0};

  auto intersection = util::computeIntersectPoint<double>(p1, p2, p3, p4);

  EXPECT_EQ(intersection.x, 0.5);
  EXPECT_EQ(intersection.y, 0.5);
}
TEST(UtilityTest, TestLeastSquare) {
  {
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{2, 4, 6, 8, 10};
    ASSERT_NEAR(least_square(x, y), 2.0, TOL);
  }
  {
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{0, 0, 0, 0, 0};
    ASSERT_NEAR(least_square(x, y), 0, TOL);
  }
  {
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{10, 8, 6, 4, 2};
    ASSERT_NEAR(least_square(x, y), -2.0, TOL);
  }
  {
    std::vector<double> x{0, 0, 0, 0, 0};
    std::vector<double> y{0, 0, 0, 0, 0};
    ASSERT_NEAR(least_square(x, y), -999.99, TOL);
  }
}