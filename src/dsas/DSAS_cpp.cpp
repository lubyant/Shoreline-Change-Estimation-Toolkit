//
// Created by lby on 10/12/23.
//
#include "DSAS_cpp.h"

#include <algorithm>
#include <iostream>

#include "../include/dsas.h"
#include "geometry.h"

namespace dsas {

void digital_shoreline_analysis_system(const Path &folder,
                                       const Path &output_path) {
  // check the input
  std::vector<Path> paths;
  try {
    if (std::filesystem::exists(folder) &&
        std::filesystem::is_directory(folder)) {
      for (const auto &entry : std::filesystem::directory_iterator(folder)) {
        if (std::filesystem::is_regular_file(entry.path())) {
          paths.push_back(entry.path());
        }
      }
    } else {
      std::cerr << "Folder is not exist!\n";
    }
  } catch (std::filesystem::filesystem_error &err) {
    std::cerr << "Error: " << err.what() << "\n";
  }
  // check the prefix
  std::string file_name_prefix = paths[0].filename().string().substr(0, 7);
  for (const auto &path : paths) {
    if (path.filename().string().substr(0, 7) != file_name_prefix) {
      throw std::runtime_error("Files are not the same image!");
    }
  }
  // create an output folder
  Path output_folder = output_path / Path(file_name_prefix);
  if (!std::filesystem::exists(output_folder)) {
    if (!std::filesystem::create_directories(output_folder)) {
      std::cerr << "cannot create the folder\n";
      exit(1);
    }
  }
  // start to analysis
  controller(paths, output_folder);
}

void digital_shoreline_analysis_system(const std::vector<Path> &paths,
                                       const Path &output_path) {
  // check the input
  std::string file_name_prefix = paths[0].filename().string().substr(0, 7);
  try {
    for (const auto &path : paths) {
      if (!std::filesystem::exists(path) ||
          !std::filesystem::is_regular_file(path)) {
        std::cerr << "Path: " << path << "not exist or not a file!\n";
      }
      // check filename has the same prefix
      if (file_name_prefix != path.filename().string().substr(0, 7)) {
        std::cerr << "Files are not the same image!" << file_name_prefix << ":"
                  << path.filename().string().substr(0, 7);
      }
    }
  } catch (std::filesystem::filesystem_error &err) {
    std::cerr << "Error: " << err.what() << "\n";
  }

  // create an output folder
  Path output_folder = output_path / Path(file_name_prefix);
  if (!std::filesystem::exists(output_folder)) {
    if (!std::filesystem::create_directories(output_folder)) {
      std::cerr << "cannot create the folder\n";
      exit(1);
    }
  }

  controller(paths, output_folder);
}

void dsas(const std::vector<Path> &folders, const Path &output_path) {
  util::ThreadPool thread_pool;
  std::vector<std::future<std::string>> futures;
  for (const auto &folder : folders) {
    auto ret = thread_pool.enqueue(
        [](const Path &folder, const Path &output) {
          dsas::digital_shoreline_analysis_system(folder, output);
          return folder.string();
        },
        folder, output_path);
    futures.push_back(std::move(ret));
  }
  for (auto &future : futures) {
    std::cout << "Task: " << future.get() << "\" complete.\n";
  }
}

void controller(const std::vector<Path> &paths, const Path &output_folder) {
  // read the image
  std::vector<std::unique_ptr<Image>> images;
  for (const auto &path : paths) {
    images.emplace_back(std::make_unique<Image>(path));
  }

  // generate the baselines
  auto baselines = generate_baselines(images);

  // generate the transects
  auto transects = generate_transects(baselines);

  // generate the intersections
  auto intersection_map = generate_intersections(images, transects);

  // compute the regression rate
  compute_rate(intersection_map, transects);

  // save the intersections to shp
  std::vector<gm::IntersectPoint> intersections;
  for (auto &kv1 : intersection_map) {
    for (auto &kv2 : kv1.second) {
      for (auto &point : kv2.second) {
        intersections.push_back(point);
      }
    }
  }
  const Path output_file_intersection =
      output_folder / Path(images[0]->file_name_ + "intersection.shp");
  util::save_points(intersections, output_file_intersection);

  // save the transects to shp
  std::vector<gm::TransectLine> output_file;
  for (auto &transect : transects) {
    for (auto &transect_line : transect.transects_) {
      output_file.push_back(std::move(transect_line));
    }
  }
  util::save_lines<gm::TransectLine>(
      output_file, output_folder / (images[0]->file_name_ + "transect.shp"));

  // save the shoreline to shp
  std::vector<gm::Shoreline> shorelines;
  for (const auto &image : images) {
    for (const auto &shoreline : image->shorelines_) {
      shorelines.push_back(shoreline);
    }
  }

  util::save_lines<gm::Shoreline>(
      shorelines, output_folder / (images[0]->file_name_ + "shoreline.shp"));

  // save the baseline to shp
  util::save_lines<gm::Baseline>(
      baselines, output_folder / (images[0]->file_name_ + "baseline.shp"));
}

Baselines generate_baselines(
    const std::vector<std::unique_ptr<Image>> &images) {
  // using the nearest the image as the baseline, since it is most eroded
  const auto& img = std::max_element(images.begin(), images.end(),
                                    [](const auto &image1, const auto &image2) {
                                      return image1->year_ < image2->year_;
                                    });

  auto &shorelines = img->get()->shorelines_;

  std::vector<gm::Baseline> baselines;
  int baseline_id{};
  for (const auto &shoreline : shorelines) {
    if(shoreline.shoreline_vertices_.empty()){
      continue;
    }
    double transect_length{500};
    double spacing{30};
    double offset{0};
    int smooth_factor{1};
    baselines.emplace_back(shoreline.shoreline_vertices_, transect_length,
                           spacing, baseline_id++, offset, smooth_factor);

  }
  return baselines;
}

TransectGroups generate_transects(const Baselines &baselines) {
  TransectGroups transectGroups{};
  for (auto &baseline : baselines) {
    Transects transects{baseline.baseline_id_, baseline.transects_lines_};
    transectGroups.push_back(transects);
  }
  return transectGroups;
}

umap<int, umap<int, std::vector<gm::IntersectPoint>>> generate_intersections(
    const std::vector<std::unique_ptr<Image>> &images,
    const TransectGroups &transectGroups) {
  umap<int, std::vector<gm::IntersectPoint>> tid_points;
  umap<int, decltype(tid_points)> bid_tid_points;
  for (const auto &image : images) {
    auto shorelines = image->shorelines_;
    auto intersections = generate_intersection(shorelines, transectGroups);
    for (const auto &intersect : intersections) {
      int baseline_id = intersect.baseline_id_;
      int transect_id = intersect.transect_id_;
      bid_tid_points[baseline_id][transect_id].push_back(intersect);
    }
  }
  return bid_tid_points;
}

std::vector<gm::IntersectPoint> generate_intersection(
    const std::vector<gm::Shoreline> &shorelines,
    const TransectGroups &transect_groups) {
  std::vector<gm::IntersectPoint> intersections;
  for (const auto &transects : transect_groups) {
    for (const auto &transectLine : transects.transects_) {
      for (const auto &shoreline : shorelines) {
        auto ret = transectLine.intersection(shoreline);
        if (ret.has_value()) {
          intersections.push_back(ret.value());
        }
      }
    }
  }
  return intersections;
}
void compute_rate(const umap<int, umap<int, std::vector<gm::IntersectPoint>>>
                      &intersection_maps,
                  TransectGroups &transect_groups) {
  // baseline_id <-> vector index
  umap<int, size_t> id_map;
  for (size_t i = 0; i < transect_groups.size(); i++) {
    id_map[transect_groups[i].baseline_id_] = i;
  }
  for (const auto &kv_baseline : intersection_maps) {
    for (const auto &kv_transect : kv_baseline.second) {
      // calculate the shoreline rate
      double reg_rate = util::linearRegressRate(kv_transect.second);

      // assign the rate to the transect
      auto it = std::find_if(
          transect_groups[id_map[kv_baseline.first]].transects_.begin(),
          transect_groups[id_map[kv_baseline.first]].transects_.end(),
          [&](const gm::TransectLine &a) {
            return a.transect_id_ == kv_transect.first;
          });
      it->change_rate = reg_rate;
    }
  }
}
}  // namespace dsas
