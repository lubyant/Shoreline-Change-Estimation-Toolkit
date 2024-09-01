//
// Created by lby on 10/21/23.
//

#include "image.hpp"

#include <queue>
#include <utility>
namespace dsas {
Image::Image(std::filesystem::path image_path, const Options &options)
    : image_path_(std::move(image_path)),
      edge_distance_(options.edge_distance),
      least_factor_(options.shoreline_least_factor),
      psz_prj_(util::get_tiff_proj(image_path_)) {
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
  std::istringstream iss(file_name_);
  iss >> image_id_;
  // extract the geographic information
  extract_geoinfo();

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
      image_id_(std::stoi(image_id)),
      edge_distance_(options.edge_distance),
      least_factor_(options.shoreline_least_factor),
      psz_prj_(util::get_tiff_proj(image_path_)) {
  year_ = static_cast<int>(date_.year());
  // extract geographic information
  extract_geoinfo();

  // extract the contour
  extract_contours();

  // extract the shorelines
  extract_shorelines();

  // process the shoreline
  process_shorelines();

  // transform the geospatial coordinate system
  transform_coordinates();
}

void Image::extract_geoinfo() {
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

  geo_info_.up_left_.x = adfGeoTransform[0];
  geo_info_.up_left_.y = adfGeoTransform[3];

  geo_info_.pixel_size_x_ = adfGeoTransform[1];
  geo_info_.pixel_size_y_ = adfGeoTransform[5];

  geo_info_.rotation_x_ = adfGeoTransform[2];
  geo_info_.rotation_y_ = adfGeoTransform[4];

  geo_info_.n_pixel_x_ = poDataset->GetRasterXSize();
  geo_info_.n_pixel_y_ = poDataset->GetRasterYSize();

  geo_info_.bottom_right_.x =
      geo_info_.up_left_.x +
      geo_info_.pixel_size_x_ * static_cast<double>(geo_info_.n_pixel_x_);
  geo_info_.bottom_right_.y =
      geo_info_.up_left_.y +
      geo_info_.pixel_size_y_ * static_cast<double>(geo_info_.n_pixel_y_);

  geo_info_.up_right_.x = geo_info_.bottom_right_.x;
  geo_info_.up_right_.y = geo_info_.up_left_.y;

  geo_info_.bottom_left_.x = geo_info_.up_left_.x;
  geo_info_.bottom_left_.y = geo_info_.bottom_right_.y;

  GDALClose(poDataset);
}

void Image::extract_contours() {
  // read the image
  cv::Mat img = cv::imread(image_path_);
  geo_info_.rows_ = img.rows;
  geo_info_.cols_ = img.cols;

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
    gm::Shoreline points{shoreline_id++, year_, image_id_, geo_info_};
    for (const auto &point : contour) {
      auto x = point.x, y = point.y;
      if (is_edge(x, y)) {
        if (!temp.empty()) {
          std::move(temp.begin(), temp.end(),
                    std::back_inserter(points.shoreline_vertices_));

          points.shoreline_vertices_ = temp;
          shorelines.push_back(std::move(points));
          points = gm::Shoreline(shoreline_id++, year_, image_id_, geo_info_);
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
  for (auto &shoreline : shorelines_) {
    size_t id{0};
    std::transform(shoreline.shoreline_vertices_.begin(),
                   shoreline.shoreline_vertices_.end(),
                   shoreline.shoreline_vertices_.begin(),
                   [&](const gm::Point<> &point) {
                     const double i = point.x, j = point.y;
                     const double X_geo = geo_info_.up_left_.x +
                                          i * geo_info_.pixel_size_x_ +
                                          j * geo_info_.rotation_x_;
                     const double Y_geo = geo_info_.up_left_.y +
                                          i * geo_info_.rotation_y_ +
                                          j * geo_info_.pixel_size_y_;
                     return gm::Point<double>(X_geo, Y_geo, id++);
                   });
  }
}

void Image::transform_coordinates(const std::string &psz_prj) {
  if (psz_prj == psz_prj_) {
    return;
  }
  GDALAllRegister();
  OGRSpatialReference sourceSRS, targetSRS;
  sourceSRS.importFromWkt(psz_prj_.c_str());
  targetSRS.importFromWkt(psz_prj.c_str());
  OGRCoordinateTransformation *coordTransform =
      OGRCreateCoordinateTransformation(&sourceSRS, &targetSRS);
  if (coordTransform == nullptr) {
    std::cerr << __LINE__ << ": Failed to create coordinate transformation."
              << std::endl;
    exit(1);
  }
  // transform geo_info
  if (coordTransform->Transform(1, &geo_info_.up_left_.x,
                                &geo_info_.up_left_.y)) {
    std::cerr << __FILE__ << ", " << __LINE__
              << "Failed to transform upper_left\n";
    exit(1);
  }
  if (coordTransform->Transform(1, &geo_info_.up_right_.x,
                                &geo_info_.up_right_.y)) {
    std::cerr << __FILE__ << ", " << __LINE__
              << "Failed to transform upper_right\n";
    exit(1);
  }
  if (coordTransform->Transform(1, &geo_info_.bottom_left_.x,
                                &geo_info_.bottom_left_.y)) {
    std::cerr << __FILE__ << ", " << __LINE__
              << "Failed to transform bottom_left\n";
    exit(1);
  }
  if (coordTransform->Transform(1, &geo_info_.bottom_right_.x,
                                &geo_info_.bottom_right_.y)) {
    std::cerr << __FILE__ << ", " << __LINE__
              << "Failed to transform bottom_right\n";
    exit(1);
  }

  // transform the shoreline coordinates
  for (auto &shoreline : shorelines_) {
    shoreline.geo_info_ = geo_info_;
    for (auto &point : shoreline.shoreline_vertices_) {
      if (!coordTransform->Transform(1, &point.x, &point.y)) {
        std::cerr << "Failed to transform point (" << point.x << ", " << point.y
                  << ")" << std::endl;
        exit(1);
      }
    }
  }
  OCTDestroyCoordinateTransformation(coordTransform);
  psz_prj_ = psz_prj;
}

std::vector<gm::Shoreline> Image::merge_shorelines_from_images(
    std::vector<Image> &images, const std::string &psz_prj) {
  // check the projection
  for (size_t i = 1; i < images.size(); i++) {
    if (images.at(i).psz_prj_ != psz_prj) {
      throw std::runtime_error(__LINE__ + "Project is not the same!\n");
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
  gm::Shorelines merge_shoreline;
  std::vector<const Image *> visited_images;
  for (const Image *image : images) {
    gm::Shorelines joint_shorelines;
    image->joint_shorelines(visited_images, &joint_shorelines);
    visited_images.push_back(image);
    if (!joint_shorelines.empty()) {
      merge_shoreline.insert(merge_shoreline.end(), joint_shorelines.begin(),
                             joint_shorelines.end());
    }
  }

  gm::Baselines baselines;
  int baseline_id{};
  for (const auto &shoreline : merge_shoreline) {
    if (shoreline.shoreline_vertices_.empty()) {
      continue;
    }
    baselines.emplace_back(shoreline.shoreline_vertices_, baseline_id++,
                           options);
  }
  return baselines;
}

bool Image::is_overlaid(const Image &image) const {
  return geo_info_.is_overlaid(image.geo_info_);
}

bool Image::is_overlaid(const gm::Point<double> &point) const {
  return geo_info_.is_overlaid(point);
}

bool Image::is_overlaid(const gm::TransectLine &transect) const {
  return geo_info_.is_overlaid(transect);
}

void Image::joint_shorelines(const std::vector<const Image *> &images,
                             gm::Shorelines *joint_shorelines) const {
  for (const auto &shoreline : shorelines_) {
    gm::Shoreline tmp_shoreline{shoreline.shoreline_id_, shoreline.year_,
                                shoreline.image_id_, shoreline.geo_info_};
    for (const auto &point : shoreline.shoreline_vertices_) {
      bool is_overlaid{false};
      for (const auto *image : images) {
        if (image->is_overlaid(point)) {
          is_overlaid = true;
        }
      }
      if (!is_overlaid) {
        tmp_shoreline.shoreline_vertices_.push_back(point);
      }
    }
    if (!tmp_shoreline.shoreline_vertices_.empty())
      joint_shorelines->push_back(std::move(tmp_shoreline));
  }
}

Image operator+(const Image &image1, const Image &image2) {
  Image image;
  if (image1.geo_info_.bottom_left_.x == -1) {
    image = image2;
    return image;
  }
  if (image2.geo_info_.bottom_left_.x == -1) {
    image = image1;
    return image;
  }
  image.geo_info_.bottom_left_ =
      gm::Point<double>(std::min(image1.geo_info_.bottom_left_.x,
                                 image2.geo_info_.bottom_left_.x),
                        std::min(image1.geo_info_.bottom_left_.y,
                                 image2.geo_info_.bottom_left_.y));
  image.geo_info_.bottom_right_ =
      gm::Point<double>(std::max(image1.geo_info_.bottom_left_.x,
                                 image2.geo_info_.bottom_left_.x),
                        std::min(image1.geo_info_.bottom_left_.y,
                                 image2.geo_info_.bottom_left_.y));
  image.geo_info_.up_left_ =
      gm::Point<double>(std::min(image1.geo_info_.bottom_left_.x,
                                 image2.geo_info_.bottom_left_.x),
                        std::max(image1.geo_info_.bottom_left_.y,
                                 image2.geo_info_.bottom_left_.y));
  image.geo_info_.up_right_ =
      gm::Point<double>(std::max(image1.geo_info_.bottom_left_.x,
                                 image2.geo_info_.bottom_left_.x),
                        std::max(image1.geo_info_.bottom_left_.y,
                                 image2.geo_info_.bottom_left_.y));
  image.shorelines_ = image1.shorelines_;
  for (const auto &shoreline : image2.shorelines_) {
    gm::Shoreline tmp_shoreline{shoreline.shoreline_id_, shoreline.year_,
                                shoreline.image_id_, shoreline.geo_info_};
    for (const auto &point : shoreline.shoreline_vertices_) {
      if (!image1.is_overlaid(point)) {
        tmp_shoreline.shoreline_vertices_.push_back(point);
      }
    }
    if (!tmp_shoreline.shoreline_vertices_.empty()) {
      image.shorelines_.push_back(std::move(tmp_shoreline));
    }
  }
  return image;
}

}  // namespace dsas
