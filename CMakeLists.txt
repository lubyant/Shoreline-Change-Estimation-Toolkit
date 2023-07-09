cmake_minimum_required(VERSION 3.22)
project(shorecalculator)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(matplotplusplus)

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
# include GDAL
include_directories(${GDAL_INCLUDE_DIRS})

# open cv
find_package(OpenCV CONFIG REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable(shorecalculator main.cpp geometry.cpp geometry.h image.cpp image.h)
target_link_libraries(shorecalculator PRIVATE ${OpenCV_LIBS})
target_link_libraries(shorecalculator PUBLIC matplot cimg)
target_link_libraries(shorecalculator PRIVATE ${GDAL_LIBRARIES})

