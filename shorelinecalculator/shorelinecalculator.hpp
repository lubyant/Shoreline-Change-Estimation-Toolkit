//
// Created by lby on 10/12/23.
//

#ifndef SHORELINECALCULATOR_SHORELINECALCULATOR_HPP
#define SHORELINECALCULATOR_SHORELINECALCULATOR_HPP

#include <filesystem>
#include <vector>

#include "geometry.hpp"
#include "image.hpp"

namespace dsas {

template <typename Key, typename Value>
using umap = std::unordered_map<Key, Value>;
using namespace gm;
using Path = std::filesystem::path;

void dsas(const std::vector<Path> &folders, const Path &output_path,
          const Options &options);

void digital_shoreline_analysis_system(const Path &folder,
                                       const Path &output_path,
                                       const Options &options);

void digital_shoreline_analysis_system(const std::vector<Path> &paths,
                                       const Path &output_path,
                                       const Options &options);

void dsas(const Path &shoreline_folder, const Path &baseline_path,
          const Path &output_transect_path,
          const Path &output_intersections_path, const Options &options);


void controller(const std::vector<Path> &paths, const Path &output_path,
                const Options &options);

Baselines generate_baselines(const std::vector<Image> &images,
                             const Options &options);

TransectGroups generate_transects(const Baselines &baselines);

umap<int, umap<int, std::vector<gm::IntersectPoint>>> generate_intersections(
    const std::vector<Image> &images,
    const TransectGroups &TransectGroups);

std::vector<gm::IntersectPoint> generate_intersection(
    const std::vector<gm::Shoreline> &shorelines,
    const TransectGroups &transect_groups);

void compute_rate(const umap<int, umap<int, std::vector<gm::IntersectPoint>>>
                      &intersections_maps,
                  TransectGroups &transect_groups,
                  double outlier_rate);

void create_transects_from_baseline(const Path &path, const Path &output_path,
                                    TransectGroups *output_transects,
                                    const Options &options);

void create_intersects_by_transects(TransectGroups &transects,
                                    const Path &shoreline_folders,
                                    const Path &output, const Options &options,
                                    const std::string &proj);


}  // namespace dsas

#endif  // SHORELINECALCULATOR_SHORELINECALCULATOR_HPP
