//
// Created by lby on 10/21/23.
//
#include "geometry.hpp"
#include "gtest/gtest.h"

#define TOL 1e-4
using namespace gm;

TEST(PointTest, test_move) {
  {
    Point<double> point{0, 0};
    point.move_point({1, 1}, 1);
    ASSERT_NEAR(point.x, sqrt(2) / 2, TOL);
    ASSERT_NEAR(point.y, sqrt(2) / 2, TOL);
  }
  {
    Point<double> point{0, 0};
    point.move_point({0, 1}, 1);
    ASSERT_NEAR(point.x, 1, TOL);
    ASSERT_NEAR(point.y, 0, TOL);
  }
}

TEST(LineTest, test_slope) {
  Point<double> left{0, 0};
  Point<double> right{1, 1};
  LineSegment lineSegment{left, right};
  ASSERT_NEAR(lineSegment.orient_, (double)45 / 180 * PI, TOL);
}

TEST(LineTest, test_move) {
  Point<double> left{0, 0};
  Point<double> right{1, 1};
  LineSegment lineSegment{left, right};
  lineSegment.move_line(1);
  ASSERT_NEAR(lineSegment.leftEdge_.x, -(double)sqrt(2) / 2, TOL);
  ASSERT_NEAR(lineSegment.leftEdge_.y, (double)sqrt(2) / 2, TOL);
  ASSERT_NEAR(lineSegment.rightEdge_.x, (double)(1 - sqrt(2) / 2), TOL);
  ASSERT_NEAR(lineSegment.rightEdge_.y, (double)(1 + sqrt(2) / 2), TOL);
}

TEST(BaselineSegTest, test_offset) {
  Point<double> left{0, 0};
  Point<double> right{0, 1};
  BaselineSeg baselineSeg{1, 1, left, right};

  ASSERT_NEAR(baselineSeg.leftEdge_.x, -1, TOL);
  ASSERT_NEAR(baselineSeg.leftEdge_.y, 0, TOL);
  ASSERT_NEAR(baselineSeg.rightEdge_.x, -1, TOL);
  ASSERT_NEAR(baselineSeg.rightEdge_.y, 1, TOL);
}

TEST(BaselineSegTest, test_transects) {
  {
    Point<double> left{0, 0};
    Point<double> right{10, 0};
    BaselineSeg baselineSeg{1, 0, left, right};

    auto transects_base_points = baselineSeg.transects_base_points_;

    ASSERT_EQ(transects_base_points.size(), 10);
  }
  {
    Point<double> left{0, 0};
    Point<double> right{10, 10};
    BaselineSeg baselineSeq{1, 1, left, right};

    auto transects_base_points = baselineSeq.transects_base_points_;

    const int num = 14;
    double x[]{0.7071067811865475, 1.414213562373095,  2.1213203435596424,
               2.82842712474619,   3.5355339059327373, 4.242640687119285,
               4.949747468305832,  5.65685424949238,   6.363961030678928,
               7.071067811865475,  7.778174593052022,  8.48528137423857,
               9.192388155425117,  9.899494936611664};
    double y[]{0.7071067811865475, 1.414213562373095,  2.1213203435596424,
               2.82842712474619,   3.5355339059327373, 4.242640687119285,
               4.949747468305832,  5.65685424949238,   6.363961030678928,
               7.071067811865475,  7.778174593052022,  8.48528137423857,
               9.192388155425117,  9.899494936611664};

    ASSERT_EQ(transects_base_points.size(), num);

    for (int i = 0; i < num; i++) {
      ASSERT_NEAR(x[i], transects_base_points[i].x, TOL);
      ASSERT_NEAR(y[i], transects_base_points[i].y, TOL);
    }
  }
}

class BaselineTest : public ::testing::Test {
 protected:
  std::unique_ptr<Baseline> baseline;

  void SetUp() override {
    BaselineSeg::cumulative_segment_distance = 0;
    BaselineSeg::cumulative_transects_distance = 0;
  }

  void TearDown() override {
    BaselineSeg::cumulative_segment_distance = 0;
    BaselineSeg::cumulative_transects_distance = 0;
  }
};

TEST_F(BaselineTest, test_baseline_transect_points1) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  gm::IntersectionMode mode{gm::IntersectionMode::Closest};
  baseline = std::make_unique<Baseline>(points, 10, 0.5, 0, 1, 0, 1, mode);
  auto transects_points = baseline->transects_base_points_;

  ASSERT_EQ(transects_points.size(), 7);
  double x[]{0, 0, 0, 0.5, 1, 1, 1};
  double y[]{0, 0.5, 1, 1, 1, 0.5, 0};

  for (size_t i = 0; i < transects_points.size(); i++) {
    ASSERT_NEAR(x[i], transects_points[i].x, TOL);
    ASSERT_NEAR(y[i], transects_points[i].y, TOL);
  }
}

TEST_F(BaselineTest, test_baseline_transect_points2) {
  gm::IntersectionMode mode{gm::IntersectionMode::Closest};
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  baseline = std::make_unique<Baseline>(points, 10, 1.5, 0, 1, 0, 1, mode);
  auto transects_points = baseline->transects_base_points_;

  ASSERT_EQ(transects_points.size(), 3);
  double x[]{0, 0.5, 1};
  double y[]{0, 1, 0};
  ASSERT_NEAR(x[0], transects_points[0].x, TOL);
  ASSERT_NEAR(x[1], transects_points[1].x, TOL);
  ASSERT_NEAR(y[0], transects_points[0].y, TOL);
  ASSERT_NEAR(y[1], transects_points[1].y, TOL);
}

TEST_F(BaselineTest, test_baseline_transect_lines) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  gm::IntersectionMode mode{gm::IntersectionMode::Closest};
  baseline = std::make_unique<Baseline>(points, 1, 0.5, 0, 1, 0, 0, mode);
  auto transects_lines = baseline->transects_lines_;

  ASSERT_EQ(transects_lines.size(), 7);
  double x[]{0.5, 0.5, 0.5, 0.5, 1, 0.5, 0.5};
  double y[]{0, 0.5, 1, 0.5, 0.5, 0.5, 0};

  for (size_t i = 0; i < transects_lines.size(); i++) {
    ASSERT_NEAR(x[i], transects_lines[i].transect_ref_point_.x, TOL);
    ASSERT_NEAR(y[i], transects_lines[i].transect_ref_point_.y, TOL);
  }
}

TEST_F(BaselineTest, test_baseline_transect_lines_right) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  gm::IntersectionMode mode{gm::IntersectionMode::Closest};
  gm::TransectOrientation orient{gm::TransectOrientation::Right};
  baseline =
      std::make_unique<Baseline>(points, 1, 0.5, 0, 1, 0, 0, mode, orient);
  auto transects_lines = baseline->transects_lines_;

  // assert if the number of transects is correct
  ASSERT_EQ(transects_lines.size(), 7);

  // assert if the transect edge is correct
  double x_right[]{1, 1, 1, 0.5, 1, 0, 0};
  double y_right[]{0, 0.5, 1, 0, 0, 0.5, 0};
  double x_left[]{0, 0, 0, 0.5, 1, 1, 1};
  double y_left[]{0, 0.5, 1, 1, 1, 0.5, 0};
  for (size_t i = 0; i < transects_lines.size(); i++) {
    ASSERT_NEAR(x_right[i], transects_lines[i].rightEdge_.x, TOL);
    ASSERT_NEAR(y_right[i], transects_lines[i].rightEdge_.y, TOL);
    ASSERT_NEAR(x_left[i], transects_lines[i].leftEdge_.x, TOL);
    ASSERT_NEAR(y_left[i], transects_lines[i].leftEdge_.y, TOL);
  }
}

TEST_F(BaselineTest, test_baseline_transect_lines_mix) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  gm::IntersectionMode mode{gm::IntersectionMode::Closest};
  gm::TransectOrientation orient{gm::TransectOrientation::Mix};
  baseline =
      std::make_unique<Baseline>(points, 1, 0.5, 0, 1, 0, 0, mode, orient);
  auto transects_lines = baseline->transects_lines_;

  // assert if the number of transects is correct
  ASSERT_EQ(transects_lines.size(), 7);

  // assert if the transect edge is correct
  double x_right[]{0.5, 0.5, 0.5, 0.5, 1, 0.5, 0.5};
  double y_right[]{0, 0.5, 1, 0.5, 0.5, 0.5, 0};
  double x_left[]{-0.5, -0.5, -0.5, 0.5, 1, 1.5, 1.5};
  double y_left[]{0, 0.5, 1, 1.5, 1.5, 0.5, 0};
  for (size_t i = 0; i < transects_lines.size(); i++) {
    ASSERT_NEAR(x_right[i], transects_lines[i].rightEdge_.x, TOL);
    ASSERT_NEAR(y_right[i], transects_lines[i].rightEdge_.y, TOL);
    ASSERT_NEAR(x_left[i], transects_lines[i].leftEdge_.x, TOL);
    ASSERT_NEAR(y_left[i], transects_lines[i].leftEdge_.y, TOL);
  }
}

TEST_F(BaselineTest, test_baseline_transect_lines_left) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  gm::IntersectionMode mode{gm::IntersectionMode::Closest};
  gm::TransectOrientation orient{gm::TransectOrientation::Left};
  baseline =
      std::make_unique<Baseline>(points, 1, 0.5, 0, 1, 0, 0, mode, orient);
  auto transects_lines = baseline->transects_lines_;

  ASSERT_EQ(transects_lines.size(), 7);

  double x_left[]{-1, -1, -1, 0.5, 1, 2, 2};
  double y_left[]{0, 0.5, 1, 2, 2, 0.5, 0};
  double x_right[]{0, 0, 0, 0.5, 1, 1, 1};
  double y_right[]{0, 0.5, 1, 1, 1, 0.5, 0};

  for (size_t i = 0; i < transects_lines.size(); i++) {
    ASSERT_NEAR(x_right[i], transects_lines[i].rightEdge_.x, TOL);
    ASSERT_NEAR(y_right[i], transects_lines[i].rightEdge_.y, TOL);
    ASSERT_NEAR(x_left[i], transects_lines[i].leftEdge_.x, TOL);
    ASSERT_NEAR(y_left[i], transects_lines[i].leftEdge_.y, TOL);
  }
}

TEST_F(BaselineTest, test_baseline_transect_length1) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  gm::IntersectionMode mode{gm::IntersectionMode::Closest};
  baseline = std::make_unique<Baseline>(points, 1, 0.5, 0, 0, 1, 0, mode);
  auto transects_lines = baseline->transects_lines_;
  for (const auto &transect : transects_lines) {
    ASSERT_NEAR(transect.leftEdge_.distance_to_point(transect.rightEdge_), 1,
                TOL);
  }
}

TEST_F(BaselineTest, test_baseline_transect_length2) {
  std::vector<Point<double>> points{{0, 0}, {1, 1}, {2, 0}, {3, 1}};
  gm::IntersectionMode mode{gm::IntersectionMode::Closest};
  baseline = std::make_unique<Baseline>(points, 1, 0.5, 0, 0, 1, 0, mode);
  auto transects_lines = baseline->transects_lines_;
  for (const auto &transect : transects_lines) {
    ASSERT_NEAR(transect.leftEdge_.distance_to_point(transect.rightEdge_), 1,
                TOL);
  }
}

TEST_F(BaselineTest, test_baseline_smooth) {
  // TODO: need more test cases
  {
    int smooth_factor{1};
    std::vector<Point<double>> points{{0, 0}, {1, 1}, {2, 0}, {3, 1}};
    gm::IntersectionMode mode{gm::IntersectionMode::Closest};
    baseline = std::make_unique<Baseline>(points, 1, 0.5, 0, 0, 1,
                                          smooth_factor, mode);
    ASSERT_EQ(baseline->baseline_vertices_.size(), 4);
    double slope{baseline->transects_lines_[0].normal_vector_.first /
                 baseline->transects_lines_[0].normal_vector_.second};
    ASSERT_NEAR(slope, 1, TOL);
  }
  {
    int smooth_factor{2};
    std::vector<Point<double>> points{{0, 0}, {1, 1}, {2, 0}, {3, 1},
                                      {3, 0}, {4, 1}, {4, 0}};
    gm::IntersectionMode mode{gm::IntersectionMode::Closest};
    baseline = std::make_unique<Baseline>(points, 1, 0.5, 0, 0, 1,
                                          smooth_factor, mode);
    ASSERT_EQ(baseline->baseline_vertices_.size(), 7);
  }
  {
    int smooth_factor{8};
    std::vector<Point<double>> points{{0, 0}, {1, 1}, {2, 0}, {3, 1},
                                      {3, 0}, {4, 1}, {4, 0}};
    gm::IntersectionMode mode{gm::IntersectionMode::Closest};
    baseline = std::make_unique<Baseline>(points, 1, 0.5, 0, 0, 1,
                                          smooth_factor, mode);
    ASSERT_EQ(baseline->baseline_vertices_.size(), 7);
  }
}