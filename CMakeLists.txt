cmake_minimum_required(VERSION 3.25)
project(shorecalculator)

set(CMAKE_CXX_STANDARD 17)
find_package(matplot++ CONFIG REQUIRED)

add_executable(shorecalculator main.cpp geometry.cpp geometry.h)
target_link_libraries(shorecalculator PRIVATE Matplot++::cimg std::filesystem Matplot++::matplot Matplot++::nodesoup)
