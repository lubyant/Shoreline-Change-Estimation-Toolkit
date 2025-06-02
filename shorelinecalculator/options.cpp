#include "options.hpp"
#include <stdexcept>
#include <string>

namespace dsas {

Options::Options(const nlohmann::json& json_value) {
  const auto& json_options = json_value.at("options");

  if (json_options.contains("smooth_factor")) {
    smooth_factor = json_options.at("smooth_factor").get<int>();
  }

  if (json_options.contains("edge_distance")) {
    edge_distance = json_options.at("edge_distance").get<int>();
  }

  if (json_options.contains("shoreline_least_factor")) {
    shoreline_least_factor = json_options.at("shoreline_least_factor").get<double>();
  }

  if (json_options.contains("transect_length")) {
    transect_length = json_options.at("transect_length").get<double>();
  }

  if (json_options.contains("transect_spacing")) {
    transect_spacing = json_options.at("transect_spacing").get<double>();
  }

  if (json_options.contains("transect_offset")) {
    transect_offset = json_options.at("transect_offset").get<double>();
  }

  if (json_options.contains("outlier_rate")) {
    outlier_rate = json_options.at("outlier_rate").get<double>();
  }

  if (json_options.contains("thread_num")) {
    thread_num = json_options.at("thread_num").get<size_t>();
  }

  if (json_options.contains("intersection_mode")) {
    std::string mode = json_options.at("intersection_mode").get<std::string>();
    if (mode == "closest") {
      intersection_mode = IntersectionMode::Closest;
    } else if (mode == "farthest") {
      intersection_mode = IntersectionMode::Farthest;
    } else {
      throw std::runtime_error("not a valid intersection mode");
    }
  }

  if (json_options.contains("transect_orientation")) {
    std::string orient = json_options.at("transect_orientation").get<std::string>();
    if (orient == "left") {
      transect_orient = TransectOrientation::Left;
    } else if (orient == "right") {
      transect_orient = TransectOrientation::Right;
    } else if (orient == "mix") {
      transect_orient = TransectOrientation::Mix;
    } else {
      throw std::runtime_error("not a valid orientation");
    }
  }
}

}  // namespace dsas
