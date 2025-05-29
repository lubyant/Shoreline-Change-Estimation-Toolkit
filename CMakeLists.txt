cmake_minimum_required(VERSION 3.22)
project(ShorelineCalculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Add Conda CMake paths
list(APPEND CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}/share/cmake")

# Find dependencies
find_package(pybind11 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(GDAL REQUIRED)
find_package(Boost REQUIRED COMPONENTS json date_time)
find_package(OpenMP REQUIRED)

# Print debug info (optional)
message(STATUS "Using OpenCV: ${OpenCV_LIBS}")
message(STATUS "Using GDAL: ${GDAL_LIBRARIES}")
message(STATUS "Using Boost: ${Boost_LIBRARIES}")

# Build core static library
add_library(DSAS STATIC
    shorelinecalculator/geometry.cpp
    shorelinecalculator/image.cpp
    shorelinecalculator/options.cpp
    shorelinecalculator/shorelinecalculator.cpp
    shorelinecalculator/utility.cpp
)
set_target_properties(DSAS PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(DSAS
    PUBLIC
        ${GDAL_LIBRARIES}
        ${OpenCV_LIBS}
        ${Boost_LIBRARIES}
        OpenMP::OpenMP_CXX
)

# Build Python extension via pybind11
pybind11_add_module(cppext shorelinecalculator/pybind.cpp)
target_link_libraries(cppext PRIVATE DSAS)
