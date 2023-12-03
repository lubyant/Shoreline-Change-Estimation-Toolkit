//
// Created by lby on 10/13/23.
//

#ifndef DSAS_CPP_IMAGE_H
#define DSAS_CPP_IMAGE_H

#include <filesystem>
#include <opencv2/opencv.hpp>
#include <string>

#include "geometry.h"
#include "utility.h"

#define IsEdge(x_cor, y_cor, x_lim, y_lim) \
  ((x_cor) == 0 || (x_cor) == x_lim || (y_cor) == 0 || (y_cor) == y_lim)

namespace dsas {

struct Image {
  using Shorelines = std::vector<gm::Shoreline>;

  // attributes
  std::filesystem::path image_path_;              // image path
  int year_{};                                    // image year
  std::string file_name_;                         // file name
  int rows_{}, cols_{};                           // image size x,y
  cv::Mat img_;                                   // image pixel vals
  std::vector<std::vector<cv::Point>> contours_;  // image edge contours
  Shorelines shorelines_;                         // shoreline contour

  Image() = delete;

  explicit Image(std::filesystem::path image_path);

  // extract the contours edges
  void extract_contours(size_t threshold);

  // extract the shorelines from the edges
  void extract_shorelines();

  // geo-transform
  void transform_coordinates();

  // check if the point is in edge
  [[nodiscard]] bool is_edge(const int x_cor, const int y_cor) const {
    return (x_cor == 0 || x_cor == cols_ || y_cor == 0 || y_cor == rows_);
  }

  [[nodiscard]] bool is_closure(const std::vector<cv::Point> &contour) const {
    return !std::any_of(contour.begin(), contour.end(),
                        [this](const cv::Point &point) {
                          return this->is_edge(point.x, point.y);
                        });
  }
};
}  // namespace dsas
#endif  // DSAS_CPP_IMAGE_H
