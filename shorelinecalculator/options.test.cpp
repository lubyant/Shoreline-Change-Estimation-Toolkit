#include "options.hpp"

#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_SUITE(OptionsTests)
BOOST_AUTO_TEST_CASE(TestDefault) {
  dsas::Options options;
  BOOST_CHECK_EQUAL(options.edge_distance, 100);
  BOOST_CHECK_EQUAL(options.shoreline_least_factor, 0.5);
  BOOST_CHECK_EQUAL(options.transect_length, 500);
  BOOST_CHECK_EQUAL(options.transect_spacing, 30);
  BOOST_CHECK_EQUAL(options.transect_offset, 0);
  BOOST_CHECK_EQUAL(options.outlier_rate, 3);
  BOOST_CHECK(options.intersection_mode == gm::IntersectionMode::Closest);
  BOOST_CHECK(options.transect_orient == gm::TransectOrientation::Mix);
  BOOST_CHECK_EQUAL(options.thread_num, std::thread::hardware_concurrency());
}
BOOST_AUTO_TEST_CASE(JsonInput){
  std::string json_string = R"(
    {
      "options":{
        "smooth_factor": 5,
        "edge_distance": 50,
        "shoreline_least_factor": 0.4,
        "transect_length": 10.0,
        "transect_spacing": 10.0,
        "transect_offset": 10.0,
        "outlier_rate": 10.0,
        "thread_num": 100,
        "intersection_mode": "farthest",
        "transect_orientation": "left"
      }
    }
  )";
  boost::json::error_code ec;
  auto json_value {boost::json::parse(json_string, ec)};
  if (ec){
    std::cerr << "json parser err\n";
    BOOST_CHECK(false);
  }
  dsas::Options options{json_value};
  BOOST_CHECK_EQUAL(options.smooth_factor, 5);
  BOOST_CHECK_EQUAL(options.edge_distance, 50);
  BOOST_CHECK_EQUAL(options.shoreline_least_factor, 0.4);
  BOOST_CHECK_EQUAL(options.transect_length, 10.0);
  BOOST_CHECK_EQUAL(options.transect_spacing, 10.0);
  BOOST_CHECK_EQUAL(options.transect_offset, 10.0);
  BOOST_CHECK_EQUAL(options.outlier_rate, 10.0);
  BOOST_CHECK_EQUAL(options.thread_num, 100);
  BOOST_CHECK(options.intersection_mode==gm::IntersectionMode::Farthest);
  BOOST_CHECK(options.transect_orient==gm::TransectOrientation::Left);
}
BOOST_AUTO_TEST_SUITE_END()