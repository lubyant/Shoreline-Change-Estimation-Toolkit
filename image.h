//
// Created by lby on 6/18/23.
//

#ifndef SHORECALCULATOR_IMAGE_H
#define SHORECALCULATOR_IMAGE_H

#include "geometry.h"
#include <filesystem>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace im {

    std::vector<std::string> read_files(std::string &path);

    std::vector<std::vector<cv::Point>>
    extract_contours_water(const std::string &path);

    std::vector<gm::Shorelines>
    extract_shorelines(const std::vector<std::vector<cv::Point>> &contours,
                       int x_lim, int y_lim, int year);

    std::vector<gm::Baselines>
    create_baseline(std::vector<gm::Shorelines> &shores_inventory,
                    double transects_length, double spacing, double offset,
                    int smooth_factor);

    std::vector<gm::Intersections>
    create_intersections(std::vector<gm::Baselines> &baselines,
                         std::vector<std::vector<gm::Shorelines>> &shorelines);

    template<typename T>
    void save_shp(std::vector<T> &shapes, const char *output_path);

} // namespace im

#endif // SHORECALCULATOR_IMAGE_H
