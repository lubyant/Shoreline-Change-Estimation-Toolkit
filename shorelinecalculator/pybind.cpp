#include <pybind11/pybind11.h>
#include <iostream>
namespace py = pybind11;
#include "shorelinecalculator.hpp"

void cppext(const std::string &input_folders, const std::string &output_folder);
void cppext(const std::string &input_folders, const std::string &output_folder){
}

PYBIND11_MODULE(cppext, m){

}