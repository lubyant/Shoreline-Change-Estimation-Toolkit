//
// Created by lby on 6/18/23.
//

#ifndef SHORECALCULATOR_IMAGE_H
#define SHORECALCULATOR_IMAGE_H
#include <string>
#include <filesystem>
#include <vector>
#include <opencv2/opencv.hpp>
#include "geometry.h"

namespace im {

    std::vector<std::string> read_files(std::string &path);
    std::vector<std::vector<cv::Point>> extract_contours_water(std::string &path);
    void
    extract_shorelines(std::vector<std::vector<cv::Point>> &contours, int x_lim, int y_lim,
                       std::vector<gm::Shorelines> &shores_inventory);
    void create_transects();
    void create_intersections();
    void save_shp();

} // im

#endif //SHORECALCULATOR_IMAGE_H
