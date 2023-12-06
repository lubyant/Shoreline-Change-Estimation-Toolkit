# 'Google_test' is the subproject name
Project(dsas_tests)
add_executable(dsas_tests
test_utility.cpp 
test_image.cpp 
test_dsas.cpp 
test_geometry.cpp)

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
target_link_libraries(dsas_tests gtest gtest_main)
target_link_libraries(dsas_tests ${OpenCV_LIBS})
target_link_libraries(dsas_tests ${GDAL_LIBRARIES})
target_link_libraries(dsas_tests dsas_lib)

add_test(NAME DsasTests COMMAND dsas_tests)