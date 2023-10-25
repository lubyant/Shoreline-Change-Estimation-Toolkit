cmake_minimum_required(VERSION 3.22)
project(DSAS_cpp)

set(CMAKE_CXX_STANDARD 17)

# opencv
find_package(OpenCV CONFIG REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )

# gtest
add_subdirectory(lib/googletest)

# gdal
find_path(GDAL_INCLUDE_DIR gdal.h PATH_SUFFIXES gdal)
find_library(GDAL_LIBRARY NAMES gdal)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GDAL DEFAULT_MSG GDAL_LIBRARY GDAL_INCLUDE_DIR)
if(GDAL_FOUND)
    set(GDAL_INCLUDE_DIRS ${GDAL_INCLUDE_DIR})
    set(GDAL_LIBRARIES ${GDAL_LIBRARY})
else()
    message(FATAL_ERROR "GDAL not found!")
endif()
include_directories(${GDAL_INCLUDE_DIRS})

# matplotplusplus
add_subdirectory(lib/matplotplusplus)

# executable
add_subdirectory(src/dsas)

# testing
add_subdirectory(src/test)

