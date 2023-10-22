//
// Created by lby on 10/21/23.
//
#include "gtest/gtest.h"
#include "../dsas/geometry.h"
#define TOL 1e-4
using namespace gm;
TEST(Line, slope){
    Point<double> left {0, 0};
    Point<double> right {1, 1};
    LineSegment lineSegment {left, right};
    ASSERT_NEAR(lineSegment.orient_, (double)45/180*PI, TOL);
}
