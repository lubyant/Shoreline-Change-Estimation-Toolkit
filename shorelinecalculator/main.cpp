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
  options.transect_orient = Options::TransectOrientation::Mix;
  options.intersection_mode = Options::IntersectionMode::Closest;
  options.outlier_metric = Options::OutlierMetric::FrechetDistance;
  digital_shoreline_analysis_system(
      Path("/home/lby/Desktop/ShorelineCalculator/Final_Folder"),
      Path("/home/lby/Desktop/ShorelineCalculator/Final_Folder_Output"),
      options);
  return 0;
}
