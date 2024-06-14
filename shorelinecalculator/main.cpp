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
  Path output_transect{"test_transects.shp"},
      output_intersects{"test_intersects.shp"};
  Path shoreline_folder{
      "/home/lby1994/ShorelineCalculator/Validation/shapefile/test.shp"};
  Path baseline_shp{
      "/home/lby1994/ShorelineCalculator/Validation/shapefile/"
      "LakeHuronBaseline.shp"};
  dsas::dsas(shoreline_folder, baseline_shp, output_transect, output_intersects,
             options);
  return 0;
}
