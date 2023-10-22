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

//    template<typename T>
//    void save_shp(std::vector<T> &shapes, const char *output_path);
    template<typename T>
    void save_shp(std::vector<T> &shapes, const char *output_path) {
        // Step 1: Initialize GDAL
        GDALAllRegister();

        // Step 2: Get the shapefile driver
        GDALDriver *driver =
                GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");

        // Step 3: Create a new shapefile
        GDALDataset *dataset =
                driver->Create(output_path, 0, 0, 0, GDT_Unknown, NULL);

        if (std::is_base_of<T, gm::Shorelines>::value) {
            // Step 4: Create a layer for the shapefile
            OGRLayer *layer = dataset->CreateLayer("line", NULL, wkbLineString, NULL);

            // Step 5: Create a new feature
            OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());

            // Step 6: Create a line geometry and add points to it
            OGRLineString line;
            for (const auto &shape: shapes) {
                for (size_t i = 0; i < shape.size(); i++) {
                    auto curPoint = shape[i];
                    line.addPoint(curPoint.x, curPoint.y);
                }
            }

            // Step 7: Add the geometry to the feature
            feature->SetGeometry(&line);

            // Step 8: Add the feature to the layer
            layer->CreateFeature(feature);
            OGRFeature::DestroyFeature(feature);
        } else {
            exit(1);
        }

        // Clean up
        GDALClose(dataset);
    }
} // namespace im

#endif // SHORECALCULATOR_IMAGE_H
