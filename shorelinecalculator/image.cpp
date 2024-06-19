//
// Created by lby on 10/21/23.
//

#include "image.hpp"

#include <queue>
#include <utility>
namespace dsas {
Image::Image(std::filesystem::path image_path, const Options &options)
    : image_path_(std::move(image_path)),
      psz_prj_(util::get_tiff_proj(image_path_)),
      edge_distance_(options.edge_distance),
      least_factor_(options.shoreline_least_factor) {
  // file name
  auto image_name = image_path_.stem().string();

  // extract the year from the name
  try {
    year_ = std::stoi(image_name.substr(image_name.size() - 4, 4));
    date_ = boost::gregorian::date(year_, 1, 1);
  } catch (std::exception &e) {
    throw std::runtime_error(image_name + e.what());
  }

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

Image::Image(std::filesystem::path image_path, std::string image_id,
             const boost::gregorian::date &date, const Options &options)
    : image_path_(std::move(image_path)),
      date_(date),
      file_name_(std::move(image_id)),
      edge_distance_(options.edge_distance),
      least_factor_(options.shoreline_least_factor) {
  year_ = static_cast<int>(date_.year());

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
  gm::Shorelines shorelines{};
  int shoreline_id{0};
  std::vector<gm::Point<double>> temp;
  for (const auto &contour : contours_) {
    gm::Shoreline points{};
    points.shoreline_id_ = shoreline_id++;
    points.year_ = year_;
    points.image_id_ = std::stoi(file_name_);
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
          points.image_id_ = std::stoi(file_name_);
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
  auto *poDataset =
      static_cast<GDALDataset *>(GDALOpen(image_path_.c_str(), GA_ReadOnly));
  if (poDataset == nullptr) {
    std::cerr << "Error opening dataset." << std::endl;
    exit(1);
  }

  double adfGeoTransform[6];
  if (poDataset->GetGeoTransform(adfGeoTransform) != CE_None) {
    std::cerr << "No geo-transform found." << std::endl;
    exit(1);
  }

  up_left_.x = adfGeoTransform[0];
  up_left_.y = adfGeoTransform[3];

  pixel_size_x_ = adfGeoTransform[1];
  pixel_size_y_ = adfGeoTransform[5];

  n_pixel_x_ = poDataset->GetRasterXSize();
  n_pixel_y_ = poDataset->GetRasterYSize();

  bottom_right_.x =
      up_left_.x + pixel_size_x_ * static_cast<double>(n_pixel_x_);
  bottom_right_.y =
      up_left_.y + pixel_size_y_ * static_cast<double>(n_pixel_y_);

  up_right_.x = bottom_right_.x;
  up_right_.y = up_left_.y;

  bottom_left_.x = up_left_.x;
  bottom_left_.y = bottom_right_.y;

  for (auto &shoreline : shorelines_) {
    std::transform(
        shoreline.shoreline_vertices_.begin(),
        shoreline.shoreline_vertices_.end(),
        shoreline.shoreline_vertices_.begin(),
        [adfGeoTransform](const gm::Point<> &point) {
          const double i = point.x, j = point.y;
          const double X_geo = adfGeoTransform[0] + i * adfGeoTransform[1] +
                               j * adfGeoTransform[2];
          const double Y_geo = adfGeoTransform[3] + i * adfGeoTransform[4] +
                               j * adfGeoTransform[5];
          return gm::Point<double>(X_geo, Y_geo);
        });
  }
  GDALClose(poDataset);
}

std::vector<gm::Shoreline> Image::merge_shorelines_from_images(
    std::vector<Image> &images) {
  // check the projection
  std::string psz_prj{images[0].psz_prj_};
  for (size_t i = 1; i < images.size(); i++) {
    if (images.at(i).psz_prj_ != psz_prj) {
      throw std::runtime_error(images[0].image_path_.string() + "-" +
                               images[i].image_path_.string() +
                               "has different projection");
    }
  }

  // merge the shorelines
  std::vector<gm::Shoreline> shorelines;
  for (auto &image : images) {
    auto &shorelines_ = image.shorelines_;
    for (auto &shoreline : shorelines_) {
      shorelines.push_back(std::move(shoreline));
    }
  }
  return shorelines;
}

gm::Baselines Image::merge_baselines_from_images(
    const std::vector<const Image *> &images, const Options &options) {
  // perform bfs
  std::vector<bool> visited(images.size(), false);
  std::queue<const Image *> q;

  Image merge_image{};

  for (size_t i = 0; i < images.size(); i++) {
    if (!visited[i]) {
      q.push(images[i]);
      visited[i] = true;
      while (!q.empty()) {
        const auto *image = q.front();
        q.pop();
        merge_image = merge_image + *image;
        for (size_t j = 0; j < images.size(); j++) {
          if (image->is_overlaid(*images[j]) && !visited[j]) {
            q.push(images[j]);
            visited[j] = true;
          }
        }
      }
    }
  }

  gm::Baselines baselines;
  int baseline_id{};
  for (const auto &shoreline : merge_image.shorelines_) {
    if (shoreline.shoreline_vertices_.empty()) {
      continue;
    }
    double transect_length{options.transect_length};
    double spacing{options.transect_spacing};
    double offset{options.transect_offset};
    int smooth_factor{options.smooth_factor};
    gm::IntersectionMode mode{options.intersection_mode};
    baselines.emplace_back(shoreline.shoreline_vertices_, transect_length,
                           spacing, baseline_id++, shoreline.image_id_, offset,
                           smooth_factor, mode);
  }
  return baselines;
}

bool Image::is_overlaid(const Image &image) const {
  const double maxX1{bottom_right_.x}, minX1{bottom_left_.x}, maxY1{up_left_.y},
      minY1{bottom_left_.y};
  const double maxX2{image.bottom_right_.x}, minX2{image.bottom_left_.x},
      maxY2{image.up_left_.y}, minY2{image.bottom_left_.y};
  const bool xOverlap = (maxX1 >= minX2) && (maxX2 >= minX1);
  const bool yOverlap = (maxY1 >= minY2) && (maxY2 >= minY1);
  return xOverlap && yOverlap;
}

bool Image::is_overlaid(const gm::Point<double> &point) const {
  bool x_overlaid = (point.x >= bottom_left_.x) && (point.x <= bottom_right_.x);
  bool y_overlaid = (point.y >= bottom_left_.y) && (point.y <= up_right_.y);
  return x_overlaid && y_overlaid;
}

Image operator+(const Image &image1, const Image &image2) {
  Image image;
  if (image1.bottom_left_.x == -1) {
    image = image2;
    return image;
  }
  if (image2.bottom_left_.x == -1) {
    image = image1;
    return image;
  }
  image.bottom_left_ =
      gm::Point<double>(std::min(image1.bottom_left_.x, image2.bottom_left_.x),
                        std::min(image1.bottom_left_.y, image2.bottom_left_.y));
  image.bottom_right_ =
      gm::Point<double>(std::max(image1.bottom_left_.x, image2.bottom_left_.x),
                        std::min(image1.bottom_left_.y, image2.bottom_left_.y));
  image.up_left_ =
      gm::Point<double>(std::min(image1.bottom_left_.x, image2.bottom_left_.x),
                        std::max(image1.bottom_left_.y, image2.bottom_left_.y));
  image.up_right_ =
      gm::Point<double>(std::max(image1.bottom_left_.x, image2.bottom_left_.x),
                        std::max(image1.bottom_left_.y, image2.bottom_left_.y));
  image.shorelines_ = image1.shorelines_;
  for (const auto &shoreline : image2.shorelines_) {
    gm::Shoreline temp_shoreline;
    temp_shoreline.shoreline_id_ = shoreline.shoreline_id_;
    temp_shoreline.year_ = shoreline.year_;
    temp_shoreline.image_id_ = shoreline.image_id_;
    for (const auto &point : shoreline.shoreline_vertices_) {
      if (!image1.is_overlaid(point)) {
        temp_shoreline.shoreline_vertices_.push_back(point);
      }
    }
    if (!temp_shoreline.shoreline_vertices_.empty()) {
      image.shorelines_.push_back(std::move(temp_shoreline));
    }
  }
  return image;
}

}  // namespace dsas
