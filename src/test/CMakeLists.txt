# 'Google_test' is the subproject name
project(test)

add_executable(test test_utility.cpp test_image.cpp test_dsas.cpp test_geometry.cpp)

# Link Google Test and main project's source files to the test executable
# opencv
find_package(OpenCV CONFIG REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
# gdal
find_path(GDAL_INCLUDE_DIR gdal.h PATH_SUFFIXES gdal)
find_library(GDAL_LIBRARY NAMES gdal)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GDAL DEFAULT_MSG GDAL_LIBRARY GDAL_INCLUDE_DIR)
if (GDAL_FOUND)
    set(GDAL_INCLUDE_DIRS ${GDAL_INCLUDE_DIR})
    set(GDAL_LIBRARIES ${GDAL_LIBRARY})
else ()
    message(FATAL_ERROR "GDAL not found!")
endif ()
include_directories(${GDAL_INCLUDE_DIRS})
target_link_libraries(test gtest gtest_main)
target_link_libraries(test ${OpenCV_LIBS})
target_link_libraries(test ${GDAL_LIBRARIES})
target_link_libraries(test dsas_lib)

enable_testing()
add_test(NAME dsas_tests COMMAND tests)