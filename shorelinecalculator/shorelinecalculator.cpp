//
// Created by lby on 10/12/23.
//
#include "shorelinecalculator.hpp"

#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "geometry.hpp"

namespace dsas {

void digital_shoreline_analysis_system(const Path &folder,
                                       const Path &output_path,
                                       const Options &options) {
  // check the input
  std::vector<Path> paths;
  try {
    if (std::filesystem::exists(folder) &&
        std::filesystem::is_directory(folder)) {
      for (const auto &entry : std::filesystem::directory_iterator(folder)) {
        for (const auto &file : std::filesystem::directory_iterator(entry)) {
          if (std::filesystem::is_regular_file(file.path())) {
            paths.push_back(file.path());
          } else {
            std::cerr << "entry: " << entry.path();
            throw std::runtime_error("no files found!");
          }
        }
      }
    } else {
      std::cerr << "Folder is not exist:" << folder << std::endl;
      exit(1);
    }
  } catch (std::filesystem::filesystem_error &err) {
    std::cerr << "Error: " << err.what() << "\n";
  }
  // check the prefix
  // create an output folder
  if (!std::filesystem::exists(output_path)) {
    if (!std::filesystem::create_directories(output_path)) {
      std::cerr << "cannot create the folder\n";
      exit(1);
    }
  }
  // start to analysis
  try {
    controller(paths, output_path, options);
  } catch (std::runtime_error &e) {
    std::cerr << e.what() << " " << folder.string() << "\n";
  }
}

void digital_shoreline_analysis_system(const std::vector<Path> &paths,
                                       const Path &output_path,
                                       const Options &options) {
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
  try {
    controller(paths, output_folder, options);
  } catch (std::runtime_error &e) {
    std::cerr << e.what() << " " << paths[0].string() << "\n";
  }
}

void dsas(const std::vector<Path> &folders, const Path &output_path,
          const Options &options) {
  GDALAllRegister();
  util::ThreadPool thread_pool(options.thread_num);

  std::vector<std::future<std::string>> futures;
  for (const auto &folder : folders) {
    auto ret = thread_pool.enqueue(
        [](const Path &folder, const Path &output, const Options &options) {
          dsas::digital_shoreline_analysis_system(folder, output, options);
          return folder.string();
        },
        folder, output_path, options);
    futures.push_back(std::move(ret));
    std::cout << "Task enqueue: " << folder.string() << "\n";
  }

  size_t volatile total_tasks{futures.size()};
  size_t volatile finished_tasks{0};
  std::unordered_set<size_t> complete_tasks_ids;
  while (true) {
    sleep(10);
    if (finished_tasks == total_tasks) {
      break;
    }
    for (size_t i = 0; i < futures.size(); i++) {
      auto &future = futures.at(i);
      if (complete_tasks_ids.find(i) == complete_tasks_ids.end() &&
          future.wait_for(std::chrono::seconds(0)) ==
              std::future_status::ready) {
        try {
          auto res{future.get()};
          finished_tasks++;
          complete_tasks_ids.insert(i);
          std::cout << finished_tasks << "/" << total_tasks << ", Task: " << res
                    << "\" complete.\n";
        } catch (const std::runtime_error &e) {
          finished_tasks++;
          std::cerr << finished_tasks << "/" << total_tasks << e.what() << ", "
                    << folders[i] << '\n';
          complete_tasks_ids.insert(i);
        } catch (const std::exception &e) {
          finished_tasks++;
          std::cerr << finished_tasks << "/" << total_tasks
                    << "Unexpected err: " << e.what() << ", " << folders[i]
                    << '\n';
          complete_tasks_ids.insert(i);
        }
      }
    }
  }
}
void dsas(const Path &shoreline_folder, const Path &baseline_path,
          const Path &output_transect_path,
          const Path &output_intersections_path, const Options &options) {
  gm::TransectGroups transect_groups;
  create_transects_from_baseline(baseline_path, output_transect_path,
                                 &transect_groups, options);
  std::cout << "transects generated.\n";
  auto proj = util::get_shp_proj(baseline_path.c_str());
  create_intersects_by_transects(transect_groups, shoreline_folder,
                                 output_intersections_path, options, proj);
  std::cout << "intersects generated.\n";
}

void controller(const std::vector<Path> &paths, const Path &output_folder,
                const Options &options) {
  // read the image
  std::vector<Image> images;
  for (const auto &path : paths) {
    try {
      images.emplace_back(path, options);
    } catch (const std::runtime_error &e) {
      std::cerr << e.what() << "\n";
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      exit(1);
    }
  }
  auto psz_prj_ = images[0].psz_prj_;

  if (images.size() <= 1) {
    throw std::runtime_error("Too few images to process: ");
  }
  std::cout << "read images: " << images.size() << std::endl;

  // generate the baselines
  auto baselines = generate_baselines(images, options);
  std::cout << "generate baselines;\n";
  auto shorelines = Image::merge_shorelines_from_images(images);

  // generate the transects
  auto transect_groups = generate_transects(baselines);
  std::cout << "generate transects;\n";

  // generate the intersections
  auto intersection_map = generate_intersections(shorelines, transect_groups);
  std::cout << "generate intersects;\n";

  // compute the regression rate
  compute_rate(intersection_map, transect_groups, options);
  std::cout << "compute the rates;\n";

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
      output_folder / Path("intersection.shp");
  util::save_points(intersections, psz_prj_.c_str(), output_file_intersection);

  // save the transects to shp
  std::vector<gm::TransectLine> output_file;
  for (auto &transect : transect_groups) {
    for (auto &transect_line : transect.transects_) {
      output_file.push_back(std::move(transect_line));
    }
  }
  util::save_lines(output_file, psz_prj_.c_str(),
                   output_folder / "transect.shp");
  util::save_points(output_file, psz_prj_.c_str(),
                    output_folder / "result.shp");

  // save the shoreline to shp
  util::save_lines<gm::Shoreline>(shorelines, psz_prj_.c_str(),
                                  output_folder / "shoreline.shp");

  // save the baseline to shp
  util::save_lines<gm::Baseline>(baselines, psz_prj_.c_str(),
                                 output_folder / "baseline.shp");
}

gm::Baselines generate_baselines(const std::vector<Image> &images,
                                 const Options &options) {
  std::unordered_map<int, std::vector<const Image *>> year_Images;
  for (const auto &image : images) {
    auto year = image.year_;
    year_Images[year].push_back(&image);
  }

  size_t lg_size = 0;
  int lg_year{};
  for (const auto &[year, image_ptrs] : year_Images) {
    if (image_ptrs.size() >= lg_size && year > lg_year) {
      lg_size = image_ptrs.size();
      lg_year = year;
    }
  }
  return Image::merge_baselines_from_images(year_Images[lg_year], options);
}

gm::TransectGroups generate_transects(gm::Baselines &baselines) {
  gm::TransectGroups transectGroups{};

  for (auto &baseline : baselines) {
    gm::Transects transects{baseline.baseline_id_, std::move(baseline.transects_lines_)};
    transectGroups.push_back(std::move(transects));
  }
  return transectGroups;
}

umap<int, umap<int, std::vector<gm::IntersectPoint>>> generate_intersections(
    const std::vector<Image> &images, gm::TransectGroups &transectGroups) {
  using tid_points_t = umap<int, std::vector<gm::IntersectPoint>>;
  umap<int, tid_points_t> bid_tid_points;
  for (const auto &image : images) {
    auto shorelines = image.shorelines_;
    auto intersections = generate_intersection(shorelines, transectGroups);
    for (const auto &intersect : intersections) {
      int baseline_id = intersect.baseline_id_;
      int transect_id = intersect.transect_id_;
      bid_tid_points[baseline_id][transect_id].push_back(intersect);
    }
  }
  return bid_tid_points;
}

umap<int, umap<int, std::vector<gm::IntersectPoint>>> generate_intersections(
    const gm::Shorelines &shorelines, gm::TransectGroups &transect_groups) {
  using tid_points_t = umap<int, std::vector<gm::IntersectPoint>>;
  umap<int, tid_points_t> bid_tid_points;
  auto intersections = generate_intersection(shorelines, transect_groups);
  for (const auto &intersect : intersections) {
    int baseline_id = intersect.baseline_id_;
    int transect_id = intersect.transect_id_;
    bid_tid_points[baseline_id][transect_id].push_back(intersect);
  }
  return bid_tid_points;
}

std::vector<gm::IntersectPoint> generate_intersection(
    const std::vector<gm::Shoreline> &shorelines,
    gm::TransectGroups &transect_groups) {
  std::vector<gm::IntersectPoint> intersections;
  for (auto &transects : transect_groups) {
    for (auto &transectLine : transects.transects_) {
      for (auto &shoreline : shorelines) {
        if (shoreline.image_id_ != transectLine.image_id_) {
          continue;
        }
        auto ret = transectLine.intersection(shoreline);
        if (ret.has_value()) {
          intersections.push_back(ret.value());
        }
      }
    }
  }
  return intersections;
}

void compute_rate(
    umap<int, umap<int, std::vector<gm::IntersectPoint>>> &intersection_maps,
    gm::TransectGroups &transect_groups, const Options &options) {
  // baseline_id <-> vector index
  umap<int, size_t> id_map;
  for (size_t i = 0; i < transect_groups.size(); i++) {
    id_map[transect_groups[i].baseline_id_] = i;
  }
  for (auto &[baseline_id, tid_map] : intersection_maps) {
    for (auto &[transect_id, intersections] : tid_map) {
      // assign the rate to the transect
      auto it =
          std::find_if(transect_groups[id_map[baseline_id]].transects_.begin(),
                       transect_groups[id_map[baseline_id]].transects_.end(),
                       [&](const gm::TransectLine &a) {
                         return a.transect_id_ == transect_id;
                       });
      // remove the duplicate intersects
      util::remove_same_year_intersections(intersections,
                                           options.intersection_mode);
      auto &transects = transect_groups[id_map[baseline_id]];
      for (auto &transect : transects.transects_) {
        if (transect.transect_id_ == transect_id) {
          for (auto &intersect : intersections) {
            transect.year_intersect_map_[intersect.year_] = &intersect;
          }
        }
      }
      for (size_t i = 0; i < transects.transects_.size(); i++) {
        if (i == 0) {
          transects.transects_[i].next_transect_line = &transects.transects_[i + 1];
        } else if (i == transects.transects_.size() - 1) {
          transects.transects_[i].prev_transect_line = &transects.transects_[i - 1];
        } else {
          transects.transects_[i].next_transect_line = &transects.transects_[i + 1];
          transects.transects_[i].prev_transect_line = &transects.transects_[i - 1];
        }
      }
      // calculate the shoreline rate
    }
  }
  
  // compute the frechet distance
  std::vector<gm::IntersectPoint> tmp_intersects;
  for (auto& transects: transect_groups){
    for(auto& transect: transects.transects_){
      transect.compute_frechet_dist();
      for(const auto& pair: transect.year_intersect_map_){
        tmp_intersects.push_back(*pair.second);
      }
      util::linearRegressRate(tmp_intersects, transect, options);
    }
  }
}
void create_transects_from_baseline(const Path &path, const Path &output_path,
                                    gm::TransectGroups *output_transects,
                                    const Options &options) {
  std::string field_name{"DSAS_ID"};
  auto baselines = util::load_baselines_shp(path, field_name, options);
  auto psz_prj_ = util::get_shp_proj(path.c_str());
  *output_transects = std::move(generate_transects(baselines));

  // output the transects
  std::vector<gm::TransectLine> output_file;
  for (auto &transect : *output_transects) {
    for (auto &transect_line : transect.transects_) {
      output_file.push_back(std::move(transect_line));
    }
  }
  util::save_lines(output_file, psz_prj_.c_str(), output_path);
}
void create_intersects_by_transects(gm::TransectGroups &transect_groups,
                                    const Path &shoreline_folders,
                                    const Path &output, const Options &options,
                                    const std::string &proj) {
  std::vector<gm::IntersectPoint> intersections;
  if (std::filesystem::is_directory(shoreline_folders)) {
    for (const auto &transect_group : transect_groups) {
      auto baseline_id{transect_group.baseline_id_};
      Path shoreline_path =
          shoreline_folders / Path(std::to_string(baseline_id)) /
          Path(std::to_string(baseline_id) + "_shoreline.shp");
      auto shorelines =
          util::load_shorelines_shp(shoreline_path, proj, baseline_id);
      for (const auto &transectLine : transect_group.transects_) {
        for (const auto &shoreline : shorelines) {
          auto ret = transectLine.intersection(shoreline);
          if (ret.has_value()) {
            intersections.push_back(ret.value());
          }
        }
      }
    }
  } else {
    for (const auto &transect_group : transect_groups) {
      const Path &shoreline_path = shoreline_folders;
      auto shorelines = util::load_shorelines_shp(shoreline_path, proj);
      for (const auto &transectLine : transect_group.transects_) {
        for (const auto &shoreline : shorelines) {
          auto ret = transectLine.intersection(shoreline);
          if (ret.has_value()) {
            intersections.push_back(ret.value());
          }
        }
      }
    }
  }

  umap<int, std::vector<gm::IntersectPoint>> tid_points;
  umap<int, decltype(tid_points)> bid_tid_points;
  for (const auto &intersect : intersections) {
    int baseline_id = intersect.baseline_id_;
    int transect_id = intersect.transect_id_;
    bid_tid_points[baseline_id][transect_id].push_back(intersect);
  }

  compute_rate(bid_tid_points, transect_groups, options);

  // save the intersections to shp
  if (intersections.empty()) {
    throw std::runtime_error("No intersections");
  }
  util::save_points(intersections, proj.c_str(), output);
}
}  // namespace dsas
