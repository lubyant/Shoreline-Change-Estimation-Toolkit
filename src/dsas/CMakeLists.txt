add_executable(imager main.cpp DSAS_cpp.cpp utility.cpp geometry.h geometry.cpp image.cpp image.h)
target_link_libraries(imager ${OpenCV_LIBS})
target_link_libraries(imager ${GDAL_LIBRARIES})