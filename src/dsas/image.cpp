//
// Created by lby on 10/21/23.
//

#include "image.h"

dsas::Image::Image(std::filesystem::path image_path)
    : image_path_(std::move(image_path)) {
  // file name
  auto image_name = image_path_.stem().string();

  // extract the year from the name
  year_ = std::stoi(image_name.substr(image_name.size() - 4, 4));

  // extract the file_name
  file_name_ = image_name.substr(0, image_name.size()-4);

  // extract the contour
  extract_contours(10);

  // extract the shorelines
  extract_shorelines();

  // transform the geosystem
  transform_coordinates();
}

void dsas::Image::extract_contours(size_t threshold) {
  // read the image
  cv::Mat img = cv::imread(image_path_);
  rows_ = img.rows;
  cols_ = img.cols;

  // grey scale
  cv::Mat gray;
  cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

  // threshold
  cv::Mat thresh;
  cv::threshold(gray, thresh, 1, 255, cv::THRESH_BINARY);

  // contour
  cv::findContours(thresh, contours_, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

  // remove contours is closure that not touch the edge
  contours_.erase(std::remove_if(contours_.begin(), contours_.end(),
                                 [this](const std::vector<cv::Point> &contour) {
                                   return this->is_closure(contour);
                                 }),
                  contours_.end());

  // remove contours that is too short
  contours_.erase(
      std::remove_if(contours_.begin(), contours_.end(),
                     [threshold](const std::vector<cv::Point> &contour) {
                       return contour.size() < threshold;
                     }),
      contours_.end());
}

void dsas::Image::extract_shorelines() {
  Shorelines shorelines{};
  int shoreline_id{};
  for (const auto &contour : contours_) {
    gm::Shoreline points{};
    points.shoreline_id_ = shoreline_id++;
    points.year_ = year_;
    for (const auto &point : contour) {
      auto x = point.x, y = point.y;
      if (!is_edge(x, y)) {
        points.shoreline_vertices_.emplace_back(x, y);
      }
    }
    shorelines.push_back(points);
  }
  shorelines_ = std::move(shorelines);
}
void dsas::Image::transform_coordinates() {
  GDALAllRegister();

  auto *poDataset = (GDALDataset *)GDALOpen(image_path_.c_str(), GA_ReadOnly);
  if (poDataset == nullptr) {
    std::cerr << "Error opening dataset." << std::endl;
    exit(1);
  }

  double adfGeoTransform[6];
  if (poDataset->GetGeoTransform(adfGeoTransform) != CE_None) {
    std::cerr << "No geotransform found." << std::endl;
    exit(1);
  }

  for (auto &shoreline : shorelines_) {
    std::transform(
        shoreline.shoreline_vertices_.begin(),
        shoreline.shoreline_vertices_.end(),
        shoreline.shoreline_vertices_.begin(),
        [adfGeoTransform](gm::Point<> &point) {
          double i = point.x, j = point.y;
          double X_geo = adfGeoTransform[0] + i * adfGeoTransform[1] +
                         j * adfGeoTransform[2];
          double Y_geo = adfGeoTransform[3] + i * adfGeoTransform[4] +
                         j * adfGeoTransform[5];
          return gm::Point<double>(X_geo, Y_geo);
        });
  }
  GDALClose(poDataset);
}
