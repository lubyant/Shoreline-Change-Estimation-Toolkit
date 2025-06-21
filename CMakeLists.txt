cmake_minimum_required(VERSION 3.22)
project(ShorelineCalculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Prefer static libraries globally
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# Set system installation paths for static GDAL/OpenCV builds
set(CMAKE_PREFIX_PATH "/usr/local")
include_directories(/usr/local/include)


# Find remaining dependencies normally
find_package(pybind11 REQUIRED)
find_package(Boost REQUIRED COMPONENTS date_time)

# Add third-party JSON header-only library
add_subdirectory(shorelinecalculator/third_party/json)

# Print diagnostics
message(STATUS "Linking static GDAL: ${GDAL_STATIC}")
message(STATUS "Linking static OpenCV core: ${OpenCV_CORE_STATIC}")
message(STATUS "Linking static OpenCV imgproc: ${OpenCV_IMGPROC_STATIC}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

# Core C++ static library
add_library(DSAS STATIC
    shorelinecalculator/geometry.cpp
    shorelinecalculator/image.cpp
    shorelinecalculator/options.cpp
    shorelinecalculator/shorelinecalculator.cpp
    shorelinecalculator/utility.cpp
)

target_include_directories(DSAS PUBLIC
    /usr/local/include
    ${Boost_INCLUDE_DIRS}
)
include_directories(/usr/local/include/opencv4)

target_link_libraries(DSAS
    PUBLIC
        ${GDAL_STATIC}
        ${OpenCV_CORE_STATIC}
        ${OpenCV_IMGPROC_STATIC}
        ${Boost_LIBRARIES}
        -lproj
        -lgeos
        -lgeos_c
        -ltiff
        -ljpeg
        -lpng
        -lsqlite3
        -lpcre
        -lexpat
        -lz
        -lcurl
        -lpthread
        -lstdc++
        -ldl
        -lm
        -fopenmp
        nlohmann_json::nlohmann_json
)

# Python extension using pybind11
pybind11_add_module(cppext MODULE shorelinecalculator/pybind.cpp)

set_target_properties(cppext PROPERTIES
    OUTPUT_NAME cppext
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shorelinecalculator
)

target_include_directories(cppext PRIVATE
    /usr/local/include
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(cppext PRIVATE DSAS)

# Install to shorelinecalculator/ package dir
install(TARGETS cppext
    LIBRARY DESTINATION shorelinecalculator
)
