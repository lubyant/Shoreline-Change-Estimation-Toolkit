cmake_minimum_required(VERSION 3.25)
project(shorecalculator)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(matplotplusplus)

find_package(OpenCV CONFIG REQUIRED)
#find_package(matplot++  REQUIRED)
#find_package(GDAL REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories('/home/lby/Downloads/gdal-3.7.0/build')

add_executable(shorecalculator main.cpp geometry.cpp geometry.h image.cpp image.h)
#target_link_libraries(shorecalculator PRIVATE Matplot++::cimg std::filesystem Matplot++::matplot Matplot++::nodesoup ${OpenCV_LIBS})
target_link_libraries(shorecalculator PRIVATE ${OpenCV_LIBS})
target_link_libraries(shorecalculator PUBLIC matplot cimg)
#target_link_libraries(shorecalculator PRIVATE GDAL::GDAL)

