//
// Created by lby on 10/12/23.
//
#include "shorelinecalculator.hpp"

#include <unistd.h>

#include <algorithm>
#include <chrono>
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

void digital_shoreline_analysis_system(const Path &image_folder,
                                       const Path &baseline_shp_path,
                                       const Path &output_path,
                                       const Options &options) {
  // check the input
  std::vector<Path> paths;
  try {
    if (std::filesystem::exists(image_folder) &&
        std::filesystem::is_directory(image_folder)) {
      for (const auto &entry :
           std::filesystem::directory_iterator(image_folder)) {
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
      std::cerr << "Folder is not exist:" << image_folder << std::endl;
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
    controller(paths, baseline_shp_path, output_path, options);
  } catch (std::runtime_error &e) {
    std::cerr << e.what() << " " << image_folder.string() << "\n";
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
  auto start = std::chrono::high_resolution_clock::now();
  // read the image
  std::vector<Image> images;
  size_t count{0};
  std::string psz_prj_;
  for (size_t i = 0; i < paths.size(); i++) {
    try {
      std::cout << ++count << "/" << paths.size() << std::endl;
      Image image{paths[i], options};
      // use the first image's coordiantes as template for all images
      if (i == 0) {
        psz_prj_ = image.psz_prj_;
      }
      // if coordinate is not consistent, transfrom
      if (image.psz_prj_ != psz_prj_) {
        image.transform_coordinates(psz_prj_);
      }
      images.push_back(std::move(image));
    } catch (const std::runtime_error &e) {
      std::cerr << e.what() << "\n";
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      exit(1);
    }
  }

  if (images.size() <= 1) {
    throw std::runtime_error("Too few images to process: ");
  }
  std::cout << "read images: " << images.size() << std::endl;
  auto end = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "elapsed time: " << elapsed.count() << std::endl;

  // generate the baselines
  start = std::chrono::high_resolution_clock::now();
  // rule: first baseline, then merge
  auto baselines = generate_baselines(images, options);
  auto shorelines = Image::merge_shorelines_from_images(images, psz_prj_);
  std::cout << "generate baselines;\n";
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "elapsed time: " << elapsed.count() << std::endl;

  // generate the transects
  start = std::chrono::high_resolution_clock::now();
  auto transect_groups = generate_transects(baselines);
  std::cout << "generate transects;\n";
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "elapsed time: " << elapsed.count() << std::endl;

  // generate the intersections
  start = std::chrono::high_resolution_clock::now();
  auto intersection_map = generate_intersections(shorelines, transect_groups);
  std::cout << "generate intersects;\n";
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "elapsed time: " << elapsed.count() << std::endl;

  // compute the regression rate
  start = std::chrono::high_resolution_clock::now();
  processes_shoreline_rate(intersection_map, transect_groups, options);
  std::cout << "process the shoreline\n";
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "elapsed time: " << elapsed.count() << std::endl;

  start = std::chrono::high_resolution_clock::now();
  frechet_distance(transect_groups, options);
  std::cout << "compute the frechet\n";
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "elapsed time: " << elapsed.count() << std::endl;

  start = std::chrono::high_resolution_clock::now();
  euc_distance(transect_groups, options);
  std::cout << "compute base distance\n";
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "elapsed time: " << elapsed.count() << std::endl;

  start = std::chrono::high_resolution_clock::now();
  compute_rate(transect_groups, options);
  std::cout << "compute the rates;\n";
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "elapsed time: " << elapsed.count() << std::endl;

  start = std::chrono::high_resolution_clock::now();
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
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::minutes>(end - start);
  std::cout << "save the files\n";
  std::cout << "elapsed time: " << elapsed.count() << std::endl;
}

void controller(const std::vector<Path> &image_paths, const Path &baseline_path,
                const Path &output_folder, const Options &options) {
  // read the image
  std::vector<Image> images;
  size_t count{0};
  std::string psz_prj_ = util::get_shp_proj(baseline_path.c_str());
  for (size_t i = 0; i < image_paths.size(); i++) {
    try {
      std::cout << ++count << "/" << image_paths.size() << std::endl;
      Image image{image_paths[i], options};
      // if coordinate is not consistent, transfrom
      if (image.psz_prj_ != psz_prj_) {
        image.transform_coordinates(psz_prj_);
      }
      images.push_back(std::move(image));
    } catch (const std::runtime_error &e) {
      std::cerr << e.what() << "\n";
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      exit(1);
    }
  }

  if (images.size() <= 1) {
    throw std::runtime_error("Too few images to process: ");
  }
  std::cout << "read images: " << images.size() << std::endl;
  auto baselines = generate_baselines(baseline_path, options);
  auto shorelines = Image::merge_shorelines_from_images(images, psz_prj_);
  auto transect_groups = generate_transects(baselines);
  auto intersection_map = generate_intersections(shorelines, transect_groups);
  processes_shoreline_rate(intersection_map, transect_groups, options);
  frechet_distance(transect_groups, options);
  euc_distance(transect_groups, options);
  compute_rate(transect_groups, options);
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
  using image_id_t = int;
  std::unordered_map<image_id_t, const Image *> id_image_map;
  for (const auto &image : images) {
    image_id_t image_id = image.image_id_;
    if (id_image_map.find(image_id) != id_image_map.end()) {
      if (id_image_map[image_id]->size() > image.size()) {
        continue;
      }
    }
    id_image_map[image_id] = &image;
  }
  std::vector<const Image *> images_selected;
  for (auto [image_id_t, image_ptr] : id_image_map) {
    images_selected.push_back(image_ptr);
  }

  return Image::merge_baselines_from_images(images_selected, options);
}

gm::Baselines generate_baselines(const Path &shp_path, const Options &options) {
  return util::load_baselines_shp(shp_path, options);
}

gm::TransectGroups generate_transects(gm::Baselines &baselines,
                                      size_t group_window) {
  gm::TransectGroups transectGroups;
  gm::Transects transect_group;
  int group_id{0};
  for (auto &baseline : baselines) {
    auto transects = baseline.set_transects();
    size_t num = transects.transects_.size() / group_window;
    for (size_t i = 0; i < num; i++) {
      for (size_t j = 0; j < group_window; j++) {
        if ((i * group_window + j) == transects.transects_.size()) {
          break;
        }
        transect_group.baseline_id_ = transects.baseline_id_;
        transects.transects_.at(i * group_window + j).group_id_ = group_id;
        transect_group.transects_.push_back(
            std::move(transects.transects_.at(i * group_window + j)));
      }
      group_id++;
      transectGroups.push_back(std::move(transect_group));
      transect_group = gm::Transects();
    }
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
        if (!shoreline.geo_info_.is_overlaid(transectLine)) {
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

void compute_rate(gm::TransectGroups &transect_groups, const Options &options) {
  // compute the distance
  std::vector<gm::IntersectPoint> tmp_intersects;
  tmp_intersects.reserve(20);
  for (auto &transects : transect_groups) {
    for (auto &transect : transects.transects_) {
      tmp_intersects.clear();
      for (const auto &pair : transect.year_intersect_map_) {
        tmp_intersects.push_back(*pair.second);
      }
      try {
        util::linearRegressRate(tmp_intersects, transect, options);
      } catch (std::runtime_error &e) {
        // std::cout << e.what() << std::endl;
      }
    }
  }
}
void create_transects_from_baseline(const Path &path, const Path &output_path,
                                    gm::TransectGroups *output_transects,
                                    const Options &options,
                                    const std::string &field_name) {
  auto baselines = util::load_baselines_shp(path, options, field_name);
  auto psz_prj_ = util::get_shp_proj(path.c_str());
  *output_transects = generate_transects(baselines);

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

  using tid_points_t = umap<int, std::vector<gm::IntersectPoint>>;
  umap<int, tid_points_t> bid_tid_points;
  for (const auto &intersect : intersections) {
    int baseline_id = intersect.baseline_id_;
    int transect_id = intersect.transect_id_;
    bid_tid_points[baseline_id][transect_id].push_back(intersect);
  }

  processes_shoreline_rate(bid_tid_points, transect_groups, options);
  compute_rate(transect_groups, options);

  // save the intersections to shp
  if (intersections.empty()) {
    throw std::runtime_error("No intersections");
  }
  util::save_points(intersections, proj.c_str(), output);
}

std::vector<gm::IntersectPoint> create_intersects_by_transects(
    gm::TransectGroups &transect_groups, const Path &shoreline_shp_path,
    const std::string &date_field_name, const Path &intersects_output_path) {
  gm::Shorelines shorelines =
      util::load_shorelines_shp(shoreline_shp_path, date_field_name.c_str());
  std::vector<gm::IntersectPoint> intersections;
  auto psz_prj_ = util::get_shp_proj(shoreline_shp_path.c_str());
  size_t num = transect_groups.size();

  std::mutex mutex;
  std::atomic<size_t> count{1};

#pragma omp parallel for
  for (int i = 0; i < static_cast<int>(transect_groups.size()); ++i) {
    std::vector<gm::IntersectPoint> local_intersections;
    const auto &transect_group = transect_groups[i];

    for (const auto &transect_line : transect_group.transects_) {
#pragma omp simd
      for (int j = 0; j < static_cast<int>(shorelines.size()); ++j) {
        const auto &shoreline = shorelines[j];
        auto ret = transect_line.intersection(shoreline);
        if (ret.has_value()) {
          local_intersections.push_back(ret.value());
        }
      }
    }

    {
      std::lock_guard<std::mutex> lock(mutex);
      intersections.insert(intersections.end(), local_intersections.begin(),
                           local_intersections.end());
      std::cout << count++ << "/" << num << std::endl;
    }
  }

  util::save_points(intersections, psz_prj_.c_str(), intersects_output_path);
  return intersections;
}

void calculate_erosion_rate(
    const std::vector<gm::IntersectPoint> &intersections,
    gm::TransectGroups &transect_groups, const std::string &output_path,
    const std::string &psz_prj_, const dsas::Options &options) {
  // group the intersections by baseline_id and group_id
  std::unordered_map<bid_t,
                     std::unordered_map<tid_t, std::vector<gm::IntersectPoint>>>
      intersect_map;
  for (const auto &intersect : intersections) {
    bid_t bid{intersect.baseline_id_};
    tid_t tid{intersect.transect_id_};
    auto &sub_set = intersect_map[bid];
    sub_set[tid].push_back(intersect);
  }
  std::vector<gm::TransectLine> results_transects;
  for (auto &transect_group : transect_groups) {
    for (auto &transect : transect_group.transects_) {
      auto bid = transect.baseline_id_;
      auto tid = transect.transect_id_;
      if(intersect_map.count(bid)){
        if(intersect_map[bid].count(tid)){
          util::linearRegressRate(intersect_map[bid][tid], transect,options);
          results_transects.push_back(transect);
        }
      }
    }
  }
  util::save_lines(results_transects, psz_prj_.c_str(), output_path);
}

void processes_shoreline_rate(intersects_maps_t &intersection_maps,
                              gm::TransectGroups &transect_groups,
                              const Options &options) {
  for (auto &transects : transect_groups) {
    for (auto &transect : transects.transects_) {
      int bid{transect.baseline_id_}, tid{transect.transect_id_};
      if (intersection_maps.find(bid) != intersection_maps.end()) {
        auto &intersections = intersection_maps[bid];
        if (intersections.find(tid) != intersections.end()) {
          auto &intersects = intersections[tid];
          util::remove_same_year_intersections(intersects,
                                               options.intersection_mode);
          for (auto &intersect : intersects) {
            transect.year_intersect_map_[intersect.year_] = &intersect;
          }
        }
      }
    }
    for (size_t i = 0; i < transects.transects_.size(); i++) {
      if (i == 0) {
        transects.transects_[i].next_transect_line =
            &transects.transects_[i + 1];
      } else if (i == transects.transects_.size() - 1) {
        transects.transects_[i].prev_transect_line =
            &transects.transects_[i - 1];
      } else {
        transects.transects_[i].next_transect_line =
            &transects.transects_[i + 1];
        transects.transects_[i].prev_transect_line =
            &transects.transects_[i - 1];
      }
    }
  }
}

void frechet_distance(gm::TransectGroups &transect_groups,
                      const Options &options) {
  // compute the distance
  using image_id_t = int;
  std::unordered_map<image_id_t, std::vector<double>> frechet_distances_map;
  for (auto &transects : transect_groups) {
    if (transects.transects_.size() < 2) {
      continue;
    }
    auto transect_first = transects.transects_[0];
    auto transect_last = transects.transects_[transects.transects_.size() - 1];
    auto shore_segments =
        util::truncate_shore_by_transects(transect_first, transect_last);
    if (shore_segments.size() < 2) {
      continue;
    }
    image_id_t image_id{shore_segments[0].image_id_};
    std::sort(shore_segments.begin(), shore_segments.end(),
              [](const gm::Shoreline &a, const gm::Shoreline &b) {
                return a.year_ < b.year_;
              });
    double dist1, dist2, fre_dist;
    for (size_t i = 0; i < shore_segments.size() - 1; i++) {
      if (shore_segments[i].shoreline_vertices_.size() < 2 ||
          shore_segments[i + 1].shoreline_vertices_.size() < 2) {
        continue;
      }
      assert(shore_segments[i].year_ != shore_segments[i + 1].year_);
      auto dur = static_cast<double>(shore_segments[i + 1].year_ -
                                     shore_segments[i].year_);
      dist1 = util::modified_frechet_distance(
          shore_segments[i].shoreline_vertices_,
          shore_segments[i + 1].shoreline_vertices_);
      std::reverse(shore_segments[i].shoreline_vertices_.begin(),
                   shore_segments[i].shoreline_vertices_.end());
      dist2 = util::modified_frechet_distance(
          shore_segments[i].shoreline_vertices_,
          shore_segments[i + 1].shoreline_vertices_);
      std::reverse(shore_segments[i + 1].shoreline_vertices_.begin(),
                   shore_segments[i + 1].shoreline_vertices_.end());

      fre_dist = std::min({dist1, dist2}) / dur;
      frechet_distances_map[image_id].push_back(fre_dist);
      for (auto &transect : transects.transects_) {
        if (transect.year_intersect_map_.find(shore_segments[i].year_) !=
            transect.year_intersect_map_.end()) {
          transect.year_intersect_map_[shore_segments[i].year_]
              ->frechet_distance_diff_ = fre_dist;
          transect.set_frechet_info(shore_segments[i].year_,
                                    shore_segments[i + 1].year_, fre_dist);
        }
      }
    }
  }
  std::unordered_map<image_id_t, double> means, stds;
  for (const auto &[image_id, fre_dists] : frechet_distances_map) {
    double mean = std::accumulate(fre_dists.begin(), fre_dists.end(), 0.0) /
                  static_cast<double>(fre_dists.size());
    double standard_dev = std::sqrt(
        std::accumulate(fre_dists.begin(), fre_dists.end(), 0.0,
                        [mean](double pre_sum, double dists) {
                          return pre_sum + (dists - mean) * (dists - mean);
                        }) /
        static_cast<double>(fre_dists.size() - 1));
    means[image_id] = mean;
    stds[image_id] = standard_dev;
  }

  for (auto &transects : transect_groups) {
    for (auto &transect : transects.transects_) {
      for (auto &[year, intersect] : transect.year_intersect_map_) {
        if (intersect->frechet_distance_diff_ == -1) {
          continue;
        }
        double cur_mean = means[intersect->image_id_];
        double cur_std = stds[intersect->image_id_];
        double cur_fre_dist{intersect->frechet_distance_diff_};
        if ((cur_fre_dist - cur_mean) > options.outlier_rate * cur_std) {
          intersect->is_fre_outlier = true;
        }
      }
    }
  }
}

void euc_distance(gm::TransectGroups &transect_groups, const Options &options) {
  using image_id_t = int;
  std::unordered_map<image_id_t, std::vector<double>> euc_distances_map;
  for (auto &transects : transect_groups) {
    std::map<int, std::vector<double>> year_dist_map;
    std::vector<int> years;
    for (auto &transect : transects.transects_) {
      auto year_intersects_map{transect.year_intersect_map_};
      for (const auto &[year, intersect] : year_intersects_map) {
        year_dist_map[year].push_back(intersect->distance_to_ref_);
      }
    }
    if (year_dist_map.size() < 2) {
      continue;
    }
    for (const auto &[year, vec] : year_dist_map) {
      years.push_back(year);
    }

    for (size_t i = 0; i < years.size() - 1; i++) {
      auto &dist_1 = year_dist_map[years[i]];
      auto &dist_2 = year_dist_map[years[i + 1]];
      double mean_dist_1 = std::accumulate(dist_1.begin(), dist_1.end(), 0.0) /
                           static_cast<double>(dist_1.size());
      double mean_dist_2 = std::accumulate(dist_2.begin(), dist_2.end(), 0.0) /
                           static_cast<double>(dist_2.size());
      double dist_rate = (mean_dist_2 - mean_dist_1) /
                         static_cast<double>(years[i + 1] - years[i]);

      for (auto &transect : transects.transects_) {
        transect.set_euc_info(years[i], years[i + 1], dist_rate);
        if (transect.year_intersect_map_.find(years[i]) !=
            transect.year_intersect_map_.end()) {
          transect.year_intersect_map_[years[i]]->euc_distance_diff = dist_rate;
        }
      }
    }
  }
  std::unordered_map<image_id_t, double> means, stds;
  for (const auto &[image_id, euc_dists] : euc_distances_map) {
    double mean = std::accumulate(euc_dists.begin(), euc_dists.end(), 0.0) /
                  static_cast<double>(euc_dists.size());
    double standard_dev = std::sqrt(
        std::accumulate(euc_dists.begin(), euc_dists.end(), 0.0,
                        [mean](double pre_sum, double dists) {
                          return pre_sum + (dists - mean) * (dists - mean);
                        }) /
        static_cast<double>(euc_dists.size() - 1));
    means[image_id] = mean;
    stds[image_id] = standard_dev;
  }
  for (auto &transects : transect_groups) {
    for (auto &transect : transects.transects_) {
      for (auto &[year, intersect] : transect.year_intersect_map_) {
        if (intersect->frechet_distance_diff_ == -1) {
          continue;
        }
        double cur_mean = means[intersect->image_id_];
        double cur_std = stds[intersect->image_id_];
        double cur_base_dist{intersect->euc_distance_diff};
        if ((cur_base_dist - cur_mean) > options.outlier_rate * cur_std) {
          intersect->is_base_outlier = true;
        }
      }
    }
  }
}
}  // namespace dsas
