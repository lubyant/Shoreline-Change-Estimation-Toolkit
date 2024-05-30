#include "options.hpp"
namespace dsas {
Options::Options(const boost::json::value &json_value) {
  auto json_options = json_value.at("options").as_object();
  if (json_options.contains("smooth_factor")) {
    smooth_factor = json_options.at("smooth_factor").as_int64();
  }

  if (json_options.contains("edge_distance")) {
    edge_distance = json_options.at("edge_distance").as_int64();
  }

  if (json_options.contains("shoreline_least_factor")) {
    shoreline_least_factor =
        json_options.at("shoreline_least_factor").as_double();
  }

  if (json_options.contains("transect_length")) {
    transect_length = json_options.at("transect_length").as_double();
  }

  if (json_options.contains("transect_spacing")) {
    transect_spacing = json_options.at("transect_spacing").as_double();
  }

  if (json_options.contains("transect_offset")) {
    transect_offset = json_options.at("transect_offset").as_double();
  }

  if (json_options.contains("outlier_rate")) {
    outlier_rate = json_options.at("outlier_rate").as_double();
  }

  if (json_options.contains("thread_num")) {
    thread_num = json_options.at("thread_num").as_int64();
  }

  if (json_options.contains("intersection_mode")) {
    auto mode = json_options.at("intersection_mode").as_string();
    if (mode == "closest") {
      intersection_mode = gm::IntersectionMode::Closest;
    } else if (mode == "farthest") {
      intersection_mode = gm::IntersectionMode::Farthest;
    } else {
      throw std::runtime_error("not a valid intersection mode");
    }
  }

  if (json_options.contains("transect_orientation")) {
    auto orient = json_options.at("transect_orientation").as_string();
    if (orient == "left") {
      transect_orient = gm::TransectOrientation::Left;
    } else if (orient == "right") {
      transect_orient = gm::TransectOrientation::Right;
    } else if (orient == "mix") {
      transect_orient = gm::TransectOrientation::Mix;
    } else {
      throw std::runtime_error("not a valid orientation");
    }
  }
}
}  // namespace dsas
