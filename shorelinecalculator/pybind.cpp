#include <pybind11/pybind11.h>
#include <iostream>
namespace py = pybind11;
#include "shorelinecalculator.hpp"

void cppext(const std::string &input_folders, const std::string &output_folder);
void cppext(const std::string &input_folders, const std::string &output_folder){
  std::cout << "cpp extenstion!\n";
}

PYBIND11_MODULE(cppext, m){
  m.doc() = "cpp extension for calculate the shoreline erosion";
  m.def("cppext", &cppext, "function take string of input folder and outputfoler");
}