//
// Created by lby on 10/21/23.
//
#include "geometry.hpp"

#include <boost/test/unit_test.hpp>

#include "options.hpp"
#include "shorelinecalculator.hpp"

#define TOL 1e-4
using namespace gm;

BOOST_AUTO_TEST_SUITE(PointTest)
BOOST_AUTO_TEST_CASE(test_move) {
  {
    Point<double> point{0, 0};
    point.move_point({1, 1}, 1);
    BOOST_CHECK_CLOSE(point.x, sqrt(2) / 2, TOL);
    BOOST_CHECK_CLOSE(point.y, sqrt(2) / 2, TOL);
  }
  {
    Point<double> point{0, 0};
    point.move_point({0, 1}, 1);
    BOOST_CHECK_CLOSE(point.x, 1, TOL);
    BOOST_CHECK_CLOSE(point.y, 0, TOL);
  }
}
BOOST_AUTO_TEST_CASE(test_slope) {
  Point<double> left{0, 0};
  Point<double> right{1, 1};
  LineSegment lineSegment{left, right};
  BOOST_CHECK_CLOSE(lineSegment.orient_, 45.0 / 180.0 * PI, TOL);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(LineTest)
BOOST_AUTO_TEST_CASE(test_move) {
  Point<double> left{0, 0};
  Point<double> right{1, 1};
  LineSegment lineSegment{left, right};
  lineSegment.move_line(1);
  BOOST_CHECK_CLOSE(lineSegment.leftEdge_.x, -(double)sqrt(2) / 2, TOL);
  BOOST_CHECK_CLOSE(lineSegment.leftEdge_.y, (double)sqrt(2) / 2, TOL);
  BOOST_CHECK_CLOSE(lineSegment.rightEdge_.x, (double)(1 - sqrt(2) / 2), TOL);
  BOOST_CHECK_CLOSE(lineSegment.rightEdge_.y, (double)(1 + sqrt(2) / 2), TOL);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(BaselineSegTest)
BOOST_AUTO_TEST_CASE(test_offset) {
  Point<double> left{0, 0};
  Point<double> right{0, 1};
  BaselineSeg baselineSeg{1, 1, left, right};

  BOOST_CHECK_CLOSE(baselineSeg.leftEdge_.x, -1, TOL);
  BOOST_CHECK_CLOSE(baselineSeg.leftEdge_.y, 0, TOL);
  BOOST_CHECK_CLOSE(baselineSeg.rightEdge_.x, -1, TOL);
  BOOST_CHECK_CLOSE(baselineSeg.rightEdge_.y, 1, TOL);
}

BOOST_AUTO_TEST_CASE(test_transects) {
  {
    Point<double> left{0, 0};
    Point<double> right{10, 0};
    BaselineSeg baselineSeg{1, 0, left, right};

    auto transects_base_points = baselineSeg.transects_base_points_;

    BOOST_CHECK_EQUAL(transects_base_points.size(), 10);
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

    BOOST_CHECK_EQUAL(transects_base_points.size(), num);

    for (int i = 0; i < num; i++) {
      BOOST_CHECK_CLOSE(x[i], transects_base_points[i].x, TOL);
      BOOST_CHECK_CLOSE(y[i], transects_base_points[i].y, TOL);
    }
  }
}
BOOST_AUTO_TEST_SUITE_END()

struct BaselineTestConfig {
  BaselineTestConfig() {
    BaselineSeg::cumulative_segment_distance = 0;
    BaselineSeg::cumulative_transects_distance = 0;
  }

  ~BaselineTestConfig() {
    BaselineSeg::cumulative_segment_distance = 0;
    BaselineSeg::cumulative_transects_distance = 0;
  }
};

BOOST_FIXTURE_TEST_SUITE(BaselineTest, BaselineTestConfig)
BOOST_AUTO_TEST_CASE(test_baseline_transect_points1) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  dsas::Options options;
  options.transect_length = 10;
  options.transect_spacing = 0.5;
  options.transect_offset = 0;
  options.smooth_factor = 1;
  options.intersection_mode = dsas::Options::IntersectionMode::Closest;
  auto baseline = std::make_unique<Baseline>(points, 0, options);
  auto transects_points = baseline->transects_base_points_;

  BOOST_CHECK_EQUAL(transects_points.size(), 7);
  double x[]{0, 0, 0, 0.5, 1, 1, 1};
  double y[]{0, 0.5, 1, 1, 1, 0.5, 0};

  for (size_t i = 0; i < transects_points.size(); i++) {
    BOOST_CHECK_CLOSE(x[i], transects_points[i].x, TOL);
    BOOST_CHECK_CLOSE(y[i], transects_points[i].y, TOL);
  }
}

BOOST_AUTO_TEST_CASE(test_baseline_transect_points2) {
  std::vector<Point<>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  dsas::Options options;
  options.transect_length = 10;
  options.transect_spacing = 1.5;
  options.transect_offset = 0;
  options.smooth_factor = 1;
  options.intersection_mode = dsas::Options::IntersectionMode::Closest;
  auto baseline = std::make_unique<Baseline>(points, 0, options);
  auto transects_points = baseline->transects_base_points_;

  BOOST_CHECK_EQUAL(transects_points.size(), 3);
  double x[]{0, 0.5, 1};
  double y[]{0, 1, 0};
  BOOST_CHECK_CLOSE(x[0], transects_points[0].x, TOL);
  BOOST_CHECK_CLOSE(x[1], transects_points[1].x, TOL);
  BOOST_CHECK_CLOSE(y[0], transects_points[0].y, TOL);
  BOOST_CHECK_CLOSE(y[1], transects_points[1].y, TOL);
}

BOOST_AUTO_TEST_CASE(test_baseline_transect_lines) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  dsas::Options options;
  options.transect_length = 1;
  options.transect_spacing = 0.5;
  options.transect_offset = 0;
  options.smooth_factor = 0;
  options.intersection_mode = dsas::Options::IntersectionMode::Closest;
  auto baseline = std::make_unique<Baseline>(points, 0, options);
  auto transects_lines = baseline->set_transects().transects_;

  BOOST_CHECK_EQUAL(transects_lines.size(), 7);
  double x[]{0.5, 0.5, 0.5, 0.5, 1, 0.5, 0.5};
  double y[]{0, 0.5, 1, 0.5, 0.5, 0.5, 0};

  for (size_t i = 0; i < transects_lines.size(); i++) {
    BOOST_CHECK_CLOSE(x[i], transects_lines[i].transect_ref_point_.x, TOL);
    BOOST_CHECK_CLOSE(y[i], transects_lines[i].transect_ref_point_.y, TOL);
  }
}

BOOST_AUTO_TEST_CASE(test_baseline_transect_lines_right) {
  std::vector<Point<>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  dsas::Options options;
  options.transect_length = 1;
  options.transect_spacing = 0.5;
  options.transect_offset = 0;
  options.smooth_factor = 0;
  options.intersection_mode = dsas::Options::IntersectionMode::Closest;
  options.transect_orient = dsas::Options::TransectOrientation::Right;
  auto baseline = std::make_unique<Baseline>(points, 0, options);
  auto transects_lines = baseline->set_transects().transects_;

  // assert if the number of transects is correct
  BOOST_CHECK_EQUAL(transects_lines.size(), 7);

  // assert if the transect edge is correct
  double x_right[]{1, 1, 1, 0.5, 1, 0, 0};
  double y_right[]{0, 0.5, 1, 0, 0, 0.5, 0};
  double x_left[]{0, 0, 0, 0.5, 1, 1, 1};
  double y_left[]{0, 0.5, 1, 1, 1, 0.5, 0};
  for (size_t i = 0; i < transects_lines.size(); i++) {
    BOOST_CHECK_CLOSE(x_right[i], transects_lines[i].rightEdge_.x, TOL);
    BOOST_CHECK_CLOSE(y_right[i], transects_lines[i].rightEdge_.y, TOL);
    BOOST_CHECK_CLOSE(x_left[i], transects_lines[i].leftEdge_.x, TOL);
    BOOST_CHECK_CLOSE(y_left[i], transects_lines[i].leftEdge_.y, TOL);
  }
}

BOOST_AUTO_TEST_CASE(test_baseline_transect_lines_mix) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  dsas::Options options;
  options.transect_length = 1;
  options.transect_spacing = 0.5;
  options.transect_offset = 0;
  options.smooth_factor = 0;
  options.intersection_mode = dsas::Options::IntersectionMode::Closest;
  options.transect_orient = dsas::Options::TransectOrientation::Mix;
  auto baseline = std::make_unique<Baseline>(points, 0, options);
  auto transects_lines = baseline->set_transects().transects_;

  // assert if the number of transects is correct
  BOOST_CHECK_EQUAL(transects_lines.size(), 7);

  // assert if the transect edge is correct
  double x_right[]{0.5, 0.5, 0.5, 0.5, 1, 0.5, 0.5};
  double y_right[]{0, 0.5, 1, 0.5, 0.5, 0.5, 0};
  double x_left[]{-0.5, -0.5, -0.5, 0.5, 1, 1.5, 1.5};
  double y_left[]{0, 0.5, 1, 1.5, 1.5, 0.5, 0};
  for (size_t i = 0; i < transects_lines.size(); i++) {
    BOOST_CHECK_CLOSE(x_right[i], transects_lines[i].rightEdge_.x, TOL);
    BOOST_CHECK_CLOSE(y_right[i], transects_lines[i].rightEdge_.y, TOL);
    BOOST_CHECK_CLOSE(x_left[i], transects_lines[i].leftEdge_.x, TOL);
    BOOST_CHECK_CLOSE(y_left[i], transects_lines[i].leftEdge_.y, TOL);
  }
}

BOOST_AUTO_TEST_CASE(test_baseline_transect_lines_left) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  dsas::Options options;
  options.transect_length = 1;
  options.transect_spacing = 0.5;
  options.transect_offset = 0;
  options.smooth_factor = 0;
  options.intersection_mode = dsas::Options::IntersectionMode::Closest;
  options.transect_orient = dsas::Options::TransectOrientation::Left;
  auto baseline = std::make_unique<Baseline>(points, 0, options);
  auto transects_lines = baseline->set_transects().transects_;

  BOOST_CHECK_EQUAL(transects_lines.size(), 7);

  double x_left[]{-1, -1, -1, 0.5, 1, 2, 2};
  double y_left[]{0, 0.5, 1, 2, 2, 0.5, 0};
  double x_right[]{0, 0, 0, 0.5, 1, 1, 1};
  double y_right[]{0, 0.5, 1, 1, 1, 0.5, 0};

  for (size_t i = 0; i < transects_lines.size(); i++) {
    BOOST_CHECK_CLOSE(x_right[i], transects_lines[i].rightEdge_.x, TOL);
    BOOST_CHECK_CLOSE(y_right[i], transects_lines[i].rightEdge_.y, TOL);
    BOOST_CHECK_CLOSE(x_left[i], transects_lines[i].leftEdge_.x, TOL);
    BOOST_CHECK_CLOSE(y_left[i], transects_lines[i].leftEdge_.y, TOL);
  }
}

BOOST_AUTO_TEST_CASE(test_baseline_transect_length1) {
  std::vector<Point<double>> points{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
  dsas::Options options;
  options.transect_length = 1;
  options.transect_spacing = 0.5;
  options.transect_offset = 1;
  options.smooth_factor = 0;
  options.intersection_mode = dsas::Options::IntersectionMode::Closest;
  auto baseline = std::make_unique<Baseline>(points, 0, options);
  auto transects_lines = baseline->set_transects().transects_;
  for (const auto &transect : transects_lines) {
    BOOST_CHECK_CLOSE(transect.leftEdge_.distance_to_point(transect.rightEdge_),
                      1, TOL);
  }
}

BOOST_AUTO_TEST_CASE(test_baseline_transect_length2) {
  std::vector<Point<double>> points{{0, 0}, {1, 1}, {2, 0}, {3, 1}};
  dsas::Options options;
  options.transect_length = 1;
  options.transect_spacing = 0.5;
  options.transect_offset = 1;
  options.smooth_factor = 0;
  options.intersection_mode = dsas::Options::IntersectionMode::Closest;
  auto baseline = std::make_unique<Baseline>(points, 0, options);
  auto transects_lines = baseline->set_transects().transects_;
  for (const auto &transect : transects_lines) {
    BOOST_CHECK_CLOSE(transect.leftEdge_.distance_to_point(transect.rightEdge_),
                      1, TOL);
  }
}

BOOST_AUTO_TEST_CASE(test_baseline_smooth) {
  // TODO: need more test cases
  {
    std::vector<Point<double>> points{{0, 0}, {1, 1}, {2, 0}, {3, 1}};
    dsas::Options options;
    options.transect_length = 1;
    options.transect_spacing = 0.5;
    options.transect_offset = 1;
    options.smooth_factor = 1;
    options.intersection_mode = dsas::Options::IntersectionMode::Closest;
    auto baseline = std::make_unique<Baseline>(points, 0, options);
    BOOST_CHECK_EQUAL(baseline->origin_vertices_.size(), 4);
    auto transect = baseline->set_transects().transects_[0];
    double slope{transect.normal_vector_.first /
                 transect.normal_vector_.second};
    BOOST_CHECK_CLOSE(slope, 1, TOL);
  }
  {
    std::vector<Point<>> points{{0, 0}, {1, 1}, {2, 0}, {3, 1},
                                {3, 0}, {4, 1}, {4, 0}};
    dsas::Options options;
    options.transect_length = 1;
    options.transect_spacing = 0.5;
    options.transect_offset = 1;
    options.smooth_factor = 2;
    options.intersection_mode = dsas::Options::IntersectionMode::Closest;
    auto baseline = std::make_unique<Baseline>(points, 0, options);
    BOOST_CHECK_EQUAL(baseline->origin_vertices_.size(), 7);
  }
  {
    std::vector<Point<double>> points{{0, 0}, {1, 1}, {2, 0}, {3, 1},
                                      {3, 0}, {4, 1}, {4, 0}};
    dsas::Options options;
    options.transect_length = 1;
    options.transect_spacing = 0.5;
    options.transect_offset = 1;
    options.smooth_factor = 8;
    options.intersection_mode = dsas::Options::IntersectionMode::Closest;
    auto baseline = std::make_unique<Baseline>(points, 0, options);
    BOOST_CHECK_EQUAL(baseline->origin_vertices_.size(), 7);
  }
}
BOOST_AUTO_TEST_CASE(test_frechetdistance) {
  std::vector<Point<>> baseline_points{{0, 0}, {1, 0}, {2, 0},
                                       {3, 0}, {4, 0}, {5, 0}};
  dsas::Options options;
  options.transect_length = 10;
  options.transect_spacing = 0.5;
  options.transect_offset = 0;
  options.smooth_factor = 10;
  options.intersection_mode = dsas::Options::IntersectionMode::Closest;
  Baseline baseline{baseline_points, 0, options};
  Baselines baselines{baseline};
  auto transect_groups = dsas::generate_transects(baselines);

  std::vector<Point<>> shoreline1_points{
      {-0.5, 1}, {1.25, 1}, {1.5, 1},  {1.75, 1}, {2.25, 1},
      {2.5, 1},  {2.75, 1}, {3.25, 1}, {3.5, 1},  {3.75, 1},
      {4.25, 1}, {4.5, 1},  {4.75, 1}};
  std::vector<Point<>> shoreline2_points{
      {-0.5, 2}, {1.25, 2}, {1.5, 2},  {1.75, 2}, {2.25, 2},
      {2.5, 2},  {2.75, 2}, {3.25, 2}, {3.5, 2},  {3.75, 2},
      {4.25, 2}, {4.5, 2},  {4.75, 2}};

  gm::GeoInfo geo_info;
  Shoreline shoreline1{shoreline1_points, 0, 2000, 0, geo_info};
  Shoreline shoreline2{shoreline2_points, 1, 2001, 0, geo_info};
  Shorelines shorelines{shoreline1, shoreline2};

  auto intersects_maps =
      dsas::generate_intersections(shorelines, transect_groups);

  processes_shoreline_rate(intersects_maps, transect_groups, options);
  frechet_distance(transect_groups, options);
  euc_distance(transect_groups, options);
  compute_rate(transect_groups, options);
  for (auto &transects : transect_groups) {
    for (auto &transect : transects.transects_) {
      transect.compute_frechet_dist();
      for (const auto dist : transect.frechet_dist_) {
        BOOST_CHECK_EQUAL(dist, 1);
      }
    }
  }
}
BOOST_AUTO_TEST_SUITE_END()