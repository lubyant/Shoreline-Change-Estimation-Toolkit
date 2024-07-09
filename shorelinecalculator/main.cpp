#include <iostream>

#include "shorelinecalculator.hpp"

int main(int argc, char **argv) {
  using namespace dsas;
  Options options;
  options.outlier_rate = 6;
  options.smooth_factor = 5;
  options.transect_spacing = 30;
  options.transect_length = 200;
  options.edge_distance = 200;
  options.shoreline_least_factor = 0.1;
  options.transect_offset = 0;
  options.transect_orient = gm::TransectOrientation::Right;
  options.intersection_mode = gm::IntersectionMode::Closest;
  dsas::digital_shoreline_analysis_system(
      Path("/home/lby/Desktop/ShorelineCalculator/METHOD_SITE"),
      Path("/home/lby/Desktop/ShorelineCalculator/METHOD_SITE_OUTPUT"), options);
  return 0;
}
