# define libray
add_library(dsas_lib
        DSAS_cpp.cpp
        geometry.cpp
        image.cpp
        utility.cpp)

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
target_link_libraries(dsas_lib ${OpenCV_LIBS})
target_link_libraries(dsas_lib ${GDAL_LIBRARIES})