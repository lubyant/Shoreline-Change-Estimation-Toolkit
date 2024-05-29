//
// Created by lby on 10/13/23.
//

#include "utility.hpp"
#include <boost/test/unit_test.hpp>
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
    BOOST_CHECK_EQUAL(1, 0);
  }
  {
    std::vector<double> data = {1, 2, 3, 100, 5, 6, -20, 8, 9, 10};
    std::vector<double> x = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    util::remove_outliers(x, data, 1.0);

    BOOST_CHECK_EQUAL(x.size(), 8);
    BOOST_CHECK_EQUAL(data.size(), 8);
  }
}

BOOST_AUTO_TEST_SUITE_END()