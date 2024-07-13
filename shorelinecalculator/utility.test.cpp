//
// Created by lby on 10/13/23.
//

#include "utility.hpp"

#include <boost/test/unit_test.hpp>

#include "shorelinecalculator.hpp"
#define TOL 1e-4
using namespace util;
BOOST_AUTO_TEST_SUITE(UtilityTest)
BOOST_AUTO_TEST_CASE(TestCrossProduct) {
  std::vector<double> vec1{0.0, 1.0};
  std::vector<double> vec2{1.0, 0.0};

  auto ans = util::crossProduct<double>(vec1, vec2);

  BOOST_CHECK_EQUAL(ans, -1);
}

BOOST_AUTO_TEST_CASE(TestIntersect) {
  using namespace gm;
  {
    Point<double> p1{0, 0};
    Point<double> p2{1, 1};
    Point<double> p3{0, 1};
    Point<double> p4{1, 0};

    BOOST_CHECK(util::isTwoSegmentIntersected<double>(p1, p2, p3, p4));
  }
  {
    Point<double> p1{0, 0};
    Point<double> p2{2, 0};
    Point<double> p3{1, -1};
    Point<double> p4{1, 1};

    BOOST_CHECK(util::isTwoSegmentIntersected<double>(p1, p2, p3, p4));
  }
}

BOOST_AUTO_TEST_CASE(TestIntersectPoint) {
  using namespace gm;
  Point<double> p1{0, 0};
  Point<double> p2{1, 1};
  Point<double> p3{0, 1};
  Point<double> p4{1, 0};

  auto intersection = util::computeIntersectPoint<double>(p1, p2, p3, p4);

  BOOST_CHECK_EQUAL(intersection.x, 0.5);
  BOOST_CHECK_EQUAL(intersection.y, 0.5);
}
BOOST_AUTO_TEST_CASE(TestLeastSquare) {
  {
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{2, 4, 6, 8, 10};
    BOOST_CHECK_CLOSE(least_square(x, y), 2.0, TOL);
  }
  {
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{0, 0, 0, 0, 0};
    BOOST_CHECK_CLOSE(least_square(x, y), 0, TOL);
  }
  {
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{10, 8, 6, 4, 2};
    BOOST_CHECK_CLOSE(least_square(x, y), -2.0, TOL);
  }
  {
    std::vector<double> x{0, 0, 0, 0, 0};
    std::vector<double> y{0, 0, 0, 0, 0};
    BOOST_CHECK_CLOSE(least_square(x, y), -999.99, TOL);
  }
}
BOOST_AUTO_TEST_CASE(TestRemoveOutliers) {
  {
    std::vector<double> data = {1, 2, 3, 100, 5, 6, -20, 8, 9, 10};
    std::vector<double> x = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    util::remove_outliers(x, data, 2.0);

    BOOST_CHECK_EQUAL(x.size(), 9);
    BOOST_CHECK_EQUAL(data.size(), 9);
  }
  {
    std::vector<double> data = {1, 2, 3, 100, 5, 6, -20, 8, 9, 10};
    std::vector<double> x = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    util::remove_outliers(x, data, 1.0);

    BOOST_CHECK_EQUAL(x.size(), 8);
    BOOST_CHECK_EQUAL(data.size(), 8);
  }
}
BOOST_AUTO_TEST_CASE(TestSubsetVertices) {
  {
    std::vector<gm::Point<>> line = {{0, 0}, {1, 2}, {2, 4},
                                     {3, 6}, {4, 8}, {5, 10}};
    gm::Point<> p1 = {1.5, 3};
    gm::Point<> p2 = {3.5, 7};
    auto subset = get_subset_of_vertices(line, p1, p2);
    std::vector<gm::Point<>> true_subset{{2, 4}, {3, 6}};
    BOOST_CHECK_EQUAL(subset.size(), 2);
    for (size_t i = 0; i < 2; i++) {
      BOOST_CHECK_CLOSE(subset[i].x, true_subset[i].x, TOL);
      BOOST_CHECK_CLOSE(subset[i].y, true_subset[i].y, TOL);
    }
  }
  {
    std::vector<gm::Point<>> line = {{0, 0}, {1, 2}, {2, 4},
                                     {3, 6}, {4, 8}, {5, 10}};
    gm::Point<> p2 = {1.5, 3};
    gm::Point<> p1 = {3.5, 7};
    auto subset = get_subset_of_vertices(line, p1, p2);
    std::vector<gm::Point<>> true_subset{{2, 4}, {3, 6}};
    BOOST_CHECK_EQUAL(subset.size(), 2);
    for (size_t i = 0; i < 2; i++) {
      BOOST_CHECK_CLOSE(subset[i].x, true_subset[i].x, TOL);
      BOOST_CHECK_CLOSE(subset[i].y, true_subset[i].y, TOL);
    }
  }
  {
    std::vector<gm::Point<>> line = {{0, 0}, {1, 2}, {2, 4},
                                     {3, 6}, {4, 8}, {5, 10}};
    std::reverse(line.begin(), line.end());
    gm::Point<> p1 = {1.5, 3};
    gm::Point<> p2 = {3.5, 7};
    auto subset = get_subset_of_vertices(line, p1, p2);
    std::vector<gm::Point<>> true_subset{{2, 4}, {3, 6}};
    std::reverse(true_subset.begin(), true_subset.end());
    BOOST_CHECK_EQUAL(subset.size(), 2);
    for (size_t i = 0; i < 2; i++) {
      BOOST_CHECK_CLOSE(subset[i].x, true_subset[i].x, TOL);
      BOOST_CHECK_CLOSE(subset[i].y, true_subset[i].y, TOL);
    }
  }
  {
    std::vector<gm::Point<>> line = {{0, 0}, {1, 2}, {2, 4},
                                     {3, 6}, {4, 8}, {5, 10}};
    std::reverse(line.begin(), line.end());
    gm::Point<> p2 = {1.5, 3};
    gm::Point<> p1 = {3.5, 7};
    auto subset = get_subset_of_vertices(line, p1, p2);
    std::vector<gm::Point<>> true_subset{{2, 4}, {3, 6}};
    std::reverse(true_subset.begin(), true_subset.end());
    BOOST_CHECK_EQUAL(subset.size(), 2);
    for (size_t i = 0; i < 2; i++) {
      BOOST_CHECK_CLOSE(subset[i].x, true_subset[i].x, TOL);
      BOOST_CHECK_CLOSE(subset[i].y, true_subset[i].y, TOL);
    }
  }
}

BOOST_AUTO_TEST_CASE(TestFrechetDistance) {
  {
    std::vector<gm::Point<>> line1{{0, 0}, {1, 1}, {2, 2}};
    std::vector<gm::Point<>> line2{{0, 0}, {1, 1}, {2, 2}};
    auto dist = frechet_distance(line1, line2);
    BOOST_CHECK_CLOSE(dist, 0, TOL);
  }
  {
    std::vector<gm::Point<>> line1{{0, 0}, {1, 1}, {2, 2}};
    std::vector<gm::Point<>> line2{{1, 1}, {2, 2}, {3, 3}};
    auto dist = frechet_distance(line1, line2);
    BOOST_CHECK_CLOSE(dist, std::sqrt(2), TOL);
  }
  {
    std::vector<gm::Point<>> line1{{0, 0}, {1, 1}, {2, 2}};
    std::vector<gm::Point<>> line2{{0, 0}, {1, 1}};
    auto dist = frechet_distance(line1, line2);
    BOOST_CHECK_CLOSE(dist, std::sqrt(2), TOL);
  }
  {
    std::vector<gm::Point<>> line1{{0, 0}, {1, 0}, {2, 0}};
    std::vector<gm::Point<>> line2{{0, 1}, {1, 1}, {2, 1}};
    auto dist = frechet_distance(line1, line2);
    BOOST_CHECK_CLOSE(dist, 1, TOL);
  }
}
BOOST_AUTO_TEST_CASE(Test_truncatebyintersect) {
  using namespace gm;
  std::vector<Point<>> baseline_points{{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
  Baseline baseline{baseline_points, 10, 0.5, 0, 10, 0, 10};
  Baselines baselines{baseline};
  dsas::Options options;
  auto transect_groups = dsas::generate_transects(baselines);

  std::vector<Point<>> shoreline1_points{
      {-0.5, 1}, {0.25, 1}, {0.5, 1}, {0.75, 1}, {1.25, 1}, {1.5, 1},
      {1.75, 1}, {2.25, 1}, {2.5, 1}, {2.75, 1}, {3.25, 1}, {3.5, 1},
      {3.75, 1}, {4.25, 1}, {4.5, 1}, {4.75, 1}};
  std::vector<Point<>> shoreline2_points{
      {-0.5, 2}, {0.25, 1}, {0.5, 1}, {0.75, 1}, {1.25, 2}, {1.5, 2},
      {1.75, 2}, {2.25, 2}, {2.5, 2}, {2.75, 2}, {3.25, 2}, {3.5, 2},
      {3.75, 2}, {4.25, 2}, {4.5, 2}, {4.75, 2}};

  Shoreline shoreline1{shoreline1_points, 0, 2000, 10};
  Shoreline shoreline2{shoreline2_points, 1, 2001, 10};
  Shorelines shorelines{shoreline1, shoreline2};

  auto intersects_maps =
      dsas::generate_intersections(shorelines, transect_groups);
  compute_rate(intersects_maps, transect_groups, options);
  for (auto &[baseline_id, maps] : intersects_maps) {
    for (auto &[transect_id, intersects] : maps) {
      for (auto &intersect : intersects) {
        auto shore_seg = util::truncate_shore_by_intersect(intersect);
        BOOST_CHECK(shore_seg.has_value());
      }
    }
  }
}
BOOST_AUTO_TEST_SUITE_END()