cmake_minimum_required(VERSION 3.10)
project(ShorelineCalculator)

set(CMAKE_CXX_STANDARD 17)


# gtest
find_package(GTest CONFIG REQUIRED)
# opencv
find_package(OpenCV CONFIG REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )
# gdal
find_package(GDAL CONFIG REQUIRED)
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
target_link_libraries(DSAS PRIVATE GDAL::GDAL)
target_link_libraries(DSAS PRIVATE Boost::json)

add_executable(${PROJECT_NAME} shorelinecalculator/main.cpp)
target_link_libraries(${PROJECT_NAME} DSAS)

enable_testing()
add_executable(${PROJECT_NAME}_tests
shorelinecalculator/utility.test.cpp 
shorelinecalculator/geometry.test.cpp)
target_link_libraries(${PROJECT_NAME}_tests GTest::gtest GTest::gtest_main)
target_link_libraries(${PROJECT_NAME}_tests ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME}_tests GDAL::GDAL)
target_link_libraries(${PROJECT_NAME}_tests DSAS)

add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)