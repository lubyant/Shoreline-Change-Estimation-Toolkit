#ifndef SHORELINECALCULATOR_OPTIONS_HPP
#define SHORELINECALCULATOR_OPTIONS_HPP

#include <thread>
#include <boost/json.hpp>

#include "geometry.hpp"

namespace dsas {
struct Options {
  enum class OutlierMetric{
    BaseDistance,
    FrechetDistance 
  };
  int smooth_factor{1};
  int edge_distance{100};
  double shoreline_least_factor{0.5};
  double transect_length{500};
  double transect_spacing{30};
  double transect_offset{0};
  double outlier_rate{3};
  OutlierMetric outlier_metric{OutlierMetric::BaseDistance};
  size_t thread_num{std::thread::hardware_concurrency()};
  gm::IntersectionMode intersection_mode{gm::IntersectionMode::Closest};
  gm::TransectOrientation transect_orient{gm::TransectOrientation::Mix};

  Options() = default;
  explicit Options(const boost::json::value &json_value);

};
}  // namespace dsas

#endif