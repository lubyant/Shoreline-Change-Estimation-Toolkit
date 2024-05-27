cmake_minimum_required(VERSION 3.10)
project(ShorelineCalculator)

set(CMAKE_CXX_STANDARD 17)


# gtest
find_package(GTest CONFIG REQUIRED)

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

# boost
find_package(Boost CONFIG REQUIRED COMPONENTS json date_time)
include_directories(${Boost_INCLUDE_DIRS})

# final executable
add_library(DSAS STATIC shorelinecalculator/geometry.cpp
        shorelinecalculator/image.cpp
        shorelinecalculator/shorelinecalculator.cpp
        shorelinecalculator/utility.cpp)
target_link_libraries(DSAS PRIVATE pthread)
target_link_libraries(DSAS PRIVATE ${OpenCV_LIBS})
target_link_libraries(DSAS PRIVATE ${GDAL_LIBRARIES})
target_link_libraries(DSAS PRIVATE Boost::json)
target_link_libraries(DSAS PRIVATE Boost::date_time)

add_executable(${PROJECT_NAME} shorelinecalculator/main.cpp)
target_link_libraries(${PROJECT_NAME} DSAS)

enable_testing()
add_executable(${PROJECT_NAME}_tests
shorelinecalculator/utility.test.cpp 
shorelinecalculator/geometry.test.cpp)
target_link_libraries(${PROJECT_NAME}_tests GTest::gtest GTest::gtest_main)
target_link_libraries(${PROJECT_NAME}_tests ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME}_tests ${GDAL_LIBRARIES})
target_link_libraries(${PROJECT_NAME}_tests DSAS)

add_test(NAME ${PROJECT_NAME}_tests COMMAND $<TARGET_FILE:${PROJECT_NAME}_tests>)
