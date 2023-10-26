#include <iostream>

#include "dsas.h"
int main(int argc, char **argv) {
  using namespace dsas;
  // no input
  if (argc == 0) {
    std::cerr << "Please input the target path!\n";
  }

  // input folder
  if (argc == 1) {
    Path input_folder{argv[0]};
    Path output_folder{argv[1]};
    digital_shoreline_analysis_system(input_folder, output_folder);
  }

  // input paths
  if (argc > 1) {
    std::vector<Path> input_paths(argc - 1);
    for (int i = 0; i < argc - 1; i++) {
      input_paths.emplace_back(argv[i]);
    }
    Path output_path{argv[argc - 1]};
    digital_shoreline_analysis_system(input_paths, output_path);
  }

  return 0;
}
