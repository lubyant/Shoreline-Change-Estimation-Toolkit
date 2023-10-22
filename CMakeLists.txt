cmake_minimum_required(VERSION 3.22)
project(shorecalculator)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(third_party/matplotplusplus)

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

# gtest
add_subdirectory(third_party/googletest-1.14.0)

include_directories(include)
file(GLOB SRC "src/*.cpp")
add_executable(shorecalculator ${SRC})
target_link_libraries(shorecalculator PRIVATE ${OpenCV_LIBS})
target_link_libraries(shorecalculator PUBLIC matplot cimg)
target_link_libraries(shorecalculator PRIVATE ${GDAL_LIBRARIES})

