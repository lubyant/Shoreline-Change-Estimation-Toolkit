//
// Created by lby on 10/13/23.
//

#ifndef SHORELINECALCULATOR_IMAGE_HPP
#define SHORELINECALCULATOR_IMAGE_HPP

#include <boost/date_time/gregorian/gregorian.hpp>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <string>

#include "geometry.hpp"
#include "options.hpp"
#include "utility.hpp"

#define IsEdge(x_cor, y_cor, x_lim, y_lim) \
  ((x_cor) == 0 || (x_cor) == x_lim || (y_cor) == 0 || (y_cor) == y_lim)

namespace dsas {
struct Image {
  // attributes
  std::filesystem::path image_path_;  // image path
  int year_{};                        // image year
  boost::gregorian::date date_;
  std::string file_name_;                         // file name
  int rows_{}, cols_{};                           // image size x,y
  std::vector<std::vector<cv::Point>> contours_;  // image edge contours
  gm::Shorelines shorelines_;                     // shoreline contour
  int edge_distance_;    // outside (ed, rows-ed) is edge
  double least_factor_;  // shoreline.size() < factor * max_size, remove
  gm::Point<double> up_left_, up_right_, bottom_left_, bottom_right_;
  double pixel_size_x_, pixel_size_y_;
  size_t n_pixel_x_, n_pixel_y_;

  Image() = delete;

  Image(std::filesystem::path image_path, const Options &options);
  Image(std::filesystem::path image_path, std::string image_id,
        const boost::gregorian::date &date, const Options &options);

  // extract the contours edges
  void extract_contours();

  // extract the shorelines from the edges
  void extract_shorelines();

  // post-process the shoreline
  void process_shorelines();

  // geo-transform
  void transform_coordinates();

  static std::vector<Shoreline> merge_shorelines_from_images()

  // check if the point is in edge
  [[nodiscard]] bool is_edge(const int x_cor, const int y_cor) const {
    int num = edge_distance_;
    return (x_cor <= num || x_cor >= cols_ - num || y_cor <= num ||
            y_cor >= rows_ - num);
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
