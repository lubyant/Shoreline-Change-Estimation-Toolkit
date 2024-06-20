#include <pybind11/pybind11.h>

#include <iostream>
namespace py = pybind11;
#include "shorelinecalculator.hpp"

void cppext(const std::string &input_folder, const std::string &output_folder,
            const dsas::Options &options);
void cppext(const std::string &input_folder, const std::string &output_folder,
            const dsas::Options &options) {
  dsas::digital_shoreline_analysis_system(input_folder, output_folder,
                                          options);
}

PYBIND11_MODULE(cppext, m) {
  m.doc() = "cpp extension for calculate the shoreline erosion\n";
  m.def("cppext", &cppext,
        "function take string of input folder and outputfoler\n");
}