cmake_minimum_required(VERSION 3.22)
project(ShorelineCalculator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable warnings for C++ files
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Og -g -fno-inline")

# Enable warnings for C files
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -g -fno-inline")

include_directories(${GDAL_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})


find_package(Boost REQUIRED COMPONENTS json date_time unit_test_framework)
set(BUILD_SHARED_LIBS OFF)
find_package(GDAL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(pybind11 REQUIRED)
find_package(OpenMP REQUIRED)

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
        OpenMP::OpenMP_CXX
)

add_executable(${PROJECT_NAME}
        shorelinecalculator/main.cpp
)

add_executable(generate_transects
        shorelinecalculator/generate_transects.cpp
)       

pybind11_add_module(cppext shorelinecalculator/pybind.cpp)
target_link_libraries(cppext PRIVATE DSAS)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        DSAS
)

target_link_libraries(
        generate_transects
        PUBLIC
        DSAS
)
enable_testing()
add_executable(${PROJECT_NAME}_tests
        shorelinecalculator/main.test.cpp
        shorelinecalculator/utility.test.cpp
        shorelinecalculator/options.test.cpp
        shorelinecalculator/image.test.cpp
        shorelinecalculator/geometry.test.cpp)

target_link_libraries(${PROJECT_NAME}_tests DSAS)
add_test(NAME ${PROJECT_NAME}_tests COMMAND $<TARGET_FILE:${PROJECT_NAME}_tests>)
