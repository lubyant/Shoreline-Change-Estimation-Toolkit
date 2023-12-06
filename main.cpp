#include <iostream>

#include "dsas.h"
int main(int argc, char **argv) {
  using namespace dsas;
  // // no input
  // if (argc == 0) {
  //   std::cerr << "Please input the target path!\n";
  // }

  // // input paths
  // if (argc > 1) {
  //   std::vector<Path> input_paths;
  //   for (int i = 1; i < argc - 1; i++) {
  //     input_paths.emplace_back(argv[i]);
  //   }
  //   Path output_path{argv[argc - 1]};
  //   dsas::dsas(input_paths, output_path);
  // }
  std::vector<Path> paths{
      "/home/lby/Desktop/shorecalculator/img/final_raster/4408504",
      "/home/lby/Desktop/shorecalculator/img/final_raster/4108603",
      "/home/lby/Desktop/shorecalculator/img/final_raster/4208607",
      "/home/lby/Desktop/shorecalculator/img/final_raster/4108730",
      "/home/lby/Desktop/shorecalculator/img/final_raster/4308734"};
  std::string output{"output"};
  Options options;
  options.smooth_factor = 1;
  options.transect_spacing = 30;
  options.transect_length = 1000;
  options.edge_distance = 200;
  options.shoreline_least_factor = 0.1;
  dsas::dsas(paths, output, options);

//   for (size_t i=0; i<paths.size(); i++)
//   digital_shoreline_analysis_system(paths[0], output, options);
  return 0;
}
