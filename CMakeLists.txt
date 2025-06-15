cmake_minimum_required(VERSION 3.22)
project(ShorelineCalculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Remove conda-specific paths
# list(APPEND CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}/share/cmake")

# Find dependencies from system paths
find_package(pybind11 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(GDAL REQUIRED)
find_package(Boost REQUIRED COMPONENTS date_time)
find_package(OpenMP REQUIRED)

# Add third-party JSON library
add_subdirectory(shorelinecalculator/third_party/json)

# Print debug info
message(STATUS "Using OpenCV: ${OpenCV_LIBS}")
message(STATUS "Using GDAL: ${GDAL_LIBRARIES}")
message(STATUS "Using GDAL include dirs: ${GDAL_INCLUDE_DIRS}")
message(STATUS "Using Boost: ${Boost_LIBRARIES}")
message(STATUS "Using pybind11: ${pybind11_INCLUDE_DIRS}")

# Build core static library
add_library(DSAS STATIC
    shorelinecalculator/geometry.cpp
    shorelinecalculator/image.cpp
    shorelinecalculator/options.cpp
    shorelinecalculator/shorelinecalculator.cpp
    shorelinecalculator/utility.cpp
)
set_target_properties(DSAS PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Add system include directories
target_include_directories(DSAS PUBLIC
    ${GDAL_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

# Link libraries to core
target_link_libraries(DSAS
    PUBLIC
        ${GDAL_LIBRARIES}
        ${OpenCV_LIBS}
        ${Boost_LIBRARIES}
        OpenMP::OpenMP_CXX
        nlohmann_json::nlohmann_json
)

# Build Python extension module using pybind11
pybind11_add_module(cppext MODULE shorelinecalculator/pybind.cpp)

set_target_properties(cppext PROPERTIES
    OUTPUT_NAME cppext
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shorelinecalculator
)

# Add include directories
target_include_directories(cppext PRIVATE
    ${GDAL_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

# Link the Python extension with the core library
target_link_libraries(cppext PRIVATE DSAS)

# Install the Python extension to the shorelinecalculator package directory
install(TARGETS cppext
    LIBRARY DESTINATION shorelinecalculator
)
