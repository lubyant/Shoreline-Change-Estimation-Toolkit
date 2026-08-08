#include <unistd.h>

#include <iostream>

#include "shorelinecalculator.hpp"

int main(int argc, char **argv) {
  using namespace dsas;
  Options options;
  options.outlier_rate = 6;
  options.smooth_factor = 10;
  options.transect_spacing = 30;
  options.transect_length = 200;
  options.edge_distance = 200;
  options.shoreline_least_factor = 0.1;
  options.transect_offset = 0;
  options.transect_orient = Options::TransectOrientation::Mix;
  options.intersection_mode = Options::IntersectionMode::Closest;
  options.outlier_metric = Options::OutlierMetric::FrechetDistance;
  if (argc == 1) {  // some hard coding input
    digital_shoreline_analysis_system(
        Path("/home/lby/Desktop/ShorelineCalculator/images"),
        Path("/home/lby/Desktop/ShorelineCalculator/image_output"), options);
    return 0;
  }
  int opt;
  Path image_folder, baseline_shp_path, output_folder;
  while ((opt = getopt(argc, argv, "f:b:o:")) != -1) {
    switch (opt) {
      case 'f':
        image_folder = optarg;
        break;
      case 'b':
        baseline_shp_path = optarg;
        break;
      case '0':
        output_folder = optarg;
        break;
      default:
        std::cerr << "Usage: " << argv[0] << " -a value -b value -c value"
                  << std::endl;
        return EXIT_FAILURE;
    }
  }
  digital_shoreline_analysis_system(image_folder, baseline_shp_path,
                                    output_folder, options);
  return 0;
}
