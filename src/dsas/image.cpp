//
// Created by lby on 10/21/23.
//

#include "image.h"
namespace dsas {
Image::Image(std::filesystem::path image_path, const Options &options)
    : image_path_(std::move(image_path)),
      edge_distance_(options.edge_distance),
      least_factor_(options.shoreline_least_factor),
      psz_prj_(set_proj()) {
  // file name
  auto image_name = image_path_.stem().string();

  // extract the year from the name
  year_ = std::stoi(image_name.substr(image_name.size() - 4, 4));

  // extract the file_name
  file_name_ = image_name.substr(0, image_name.size() - 4);

  // extract the contour
  extract_contours();

  // extract the shorelines
  extract_shorelines();

  // process the shoreline
  process_shorelines();

  // transform the geospatial coordinate system
  transform_coordinates();
}

void Image::extract_contours() {
  // read the image
  cv::Mat img = cv::imread(image_path_);
  rows_ = img.rows;
  cols_ = img.cols;

  // grey scale
  cv::Mat gray;
  cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

  // threshold
  cv::Mat thresh;
  cv::threshold(gray, thresh, 1, 255, 0);

  // contour
  cv::findContours(thresh, contours_, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

  if (contours_.empty()) {
    throw std::runtime_error("No contours in images: " + image_path_.string());
  }

  // remove contours is closure that not touch the edge
  contours_.erase(std::remove_if(contours_.begin(), contours_.end(),
                                 [this](const std::vector<cv::Point> &contour) {
                                   return this->is_closure(contour);
                                 }),
                  contours_.end());
  if (contours_.empty()) {
    throw std::runtime_error("No edge contours in images" +
                             image_path_.string());
  }
}

void Image::extract_shorelines() {
  Shorelines shorelines{};
  int shoreline_id{0};
  std::vector<gm::Point<double>> temp;
  for (const auto &contour : contours_) {
    gm::Shoreline points{};
    points.shoreline_id_ = shoreline_id++;
    points.year_ = year_;
    for (const auto &point : contour) {
      auto x = point.x, y = point.y;
      if (is_edge(x, y)) {
        if (!temp.empty()) {
          std::move(temp.begin(), temp.end(),
                    std::back_inserter(points.shoreline_vertices_));

          points.shoreline_vertices_ = temp;
          shorelines.push_back(std::move(points));
          points = gm::Shoreline();
          points.shoreline_id_ = shoreline_id++;
          points.year_ = year_;
          temp.clear();
        }
        continue;
      }
      temp.emplace_back(x, y);
    }
    if (!temp.empty()) {
      std::move(temp.begin(), temp.end(),
                std::back_inserter(points.shoreline_vertices_));
      shorelines.push_back(points);
    }
  }
  if (shorelines.empty()) {
    throw std::runtime_error("No shorelines from contours" +
                             image_path_.string());
  }
  shorelines_ = std::move(shorelines);
}

void Image::process_shorelines() {
  // find out the maximum shoreline length
  auto max_num = std::max_element(
      shorelines_.begin(), shorelines_.end(),
      [](const auto &a, const auto &b) { return a.size() < b.size(); });
  if (max_num == shorelines_.end()) {
    throw std::runtime_error("No shorelines available.");
  }

  // threshold = max * least_factor
  auto threshold =
      (size_t)(least_factor_ * static_cast<double>(max_num->size()));

  // remove shorelines that is too short
  shorelines_.erase(
      std::remove_if(shorelines_.begin(), shorelines_.end(),
                     [threshold](const auto &shoreline) {
                       return shoreline.shoreline_vertices_.size() < threshold;
                     }),
      shorelines_.end());
}

void Image::transform_coordinates() {
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
}  // namespace dsas
