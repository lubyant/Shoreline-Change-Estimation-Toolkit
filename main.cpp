#include <iostream>

#include "dsas.h"
int main(int argc, char **argv) {
  using namespace dsas;
  // no input
  if (argc == 0) {
    std::cerr << "Please input the target path!\n";
  }

  // input paths
  if (argc > 1) {
    std::vector<Path> input_paths;
    for (int i = 1; i < argc - 1; i++) {
      input_paths.emplace_back(argv[i]);
    }
    Path output_path{argv[argc - 1]};
    dsas::dsas(input_paths, output_path);
  }

  return 0;
}
