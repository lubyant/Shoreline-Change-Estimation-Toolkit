
#include <pybind11/pybind11.h>
#include "shorelinecalculator.hpp"
int add(int a, int b);
int add(int a, int b){
    return a+b;
}

namespace py = pybind11;
PYBIND11_MODULE(example, m) {
    m.doc() = "pybind11 example plugin"; // Optional module docstring

    m.def("add", &add, "A function which adds two numbers");
}
