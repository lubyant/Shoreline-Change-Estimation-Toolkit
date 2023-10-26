cmake_minimum_required(VERSION 3.22)
project(DSAS_cpp)

set(CMAKE_CXX_STANDARD 17)

# include directory
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/lib)
# add subdirectory
add_subdirectory(src/dsas)
add_subdirectory(src/test)


# gtest
add_subdirectory(lib/googletest)

# opencv
find_package(OpenCV CONFIG REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )
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


# final executable
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} dsas_lib)