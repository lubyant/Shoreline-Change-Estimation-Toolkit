cmake_minimum_required(VERSION 3.22)
project(ShorelineCalculator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
include_directories(${GDAL_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})


find_package(Boost REQUIRED COMPONENTS json date_time unit_test_framework)
find_package(GDAL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(pybind11 REQUIRED)

add_library(DSAS STATIC
    shorelinecalculator/geometry.cpp
    shorelinecalculator/image.cpp
    shorelinecalculator/options.cpp
    shorelinecalculator/shorelinecalculator.cpp
    shorelinecalculator/utility.cpp)
target_link_libraries(
    DSAS 
    PUBLIC
    pthread
    ${GDAL_LIBRARIES} ${OpenCV_LIBS} ${Boost_LIBRARIES} 
)

add_executable(${PROJECT_NAME} 
shorelinecalculator/main.cpp
)

pybind11_add_module(example_module shorelinecalculator/cpp_interface.cpp)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    DSAS
)
enable_testing()
add_executable(${PROJECT_NAME}_tests
    shorelinecalculator/main.test.cpp
    shorelinecalculator/utility.test.cpp 
    shorelinecalculator/options.test.cpp
    shorelinecalculator/geometry.test.cpp)

target_link_libraries(${PROJECT_NAME}_tests DSAS)
add_test(NAME ${PROJECT_NAME}_tests COMMAND $<TARGET_FILE:${PROJECT_NAME}_tests>)
