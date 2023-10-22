# 'Google_test' is the subproject name
project(test)

add_executable(test test_utility.cpp test_image.cpp test_dsas.cpp
        ../dsas/image.cpp ../dsas/geometry.cpp ../dsas/utility.cpp test_geometry.cpp)

# Link Google Test and main project's source files to the test executable
target_link_libraries(test gtest gtest_main)
target_link_libraries(test ${OpenCV_LIBS})
target_link_libraries(test ${GDAL_LIBRARIES})
