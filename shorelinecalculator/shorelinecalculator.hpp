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
using Path = std::filesystem::path;

void dsas(const std::vector<Path> &folders, const Path &output_path,
          const Options &options);

void digital_shoreline_analysis_system(const Path &image_folder,
                                       const Path &output_path,
                                       const Options &options);

void digital_shoreline_analysis_system(const Path &image_folder,
                                       const Path &baseline_shp_path,
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

void controller(const std::vector<Path> &image_paths, const Path &baseline_path,
                const Path &output_path, const Options &options);

gm::Baselines generate_baselines(const std::vector<Image> &images,
                                 const Options &options);

gm::Baselines generate_baselines(const Path &shp_path, const Options &options);

gm::TransectGroups generate_transects(gm::Baselines &baselines,
                                      size_t group_window = 10);

umap<int, umap<int, std::vector<gm::IntersectPoint>>> generate_intersections(
    const std::vector<Image> &images, gm::TransectGroups &TransectGroups);

umap<int, umap<int, std::vector<gm::IntersectPoint>>> generate_intersections(
    const gm::Shorelines &shorelines, gm::TransectGroups &transect_groups);

std::vector<gm::IntersectPoint> generate_intersection(
    const gm::Shorelines &shorelines, gm::TransectGroups &transect_groups);

using bid_t = int;  // baseline id
using tid_t = int;  // transect id
using intersects_maps_t =
    umap<bid_t, umap<tid_t, std::vector<gm::IntersectPoint>>>;

void compute_rate(gm::TransectGroups &transect_groups, const Options &options);

void processes_shoreline_rate(intersects_maps_t &intersection_maps,
                              gm::TransectGroups &transect_groups,
                              const Options &options);

void frechet_distance(gm::TransectGroups &transect_groups,
                      const Options &options);

void euc_distance(gm::TransectGroups &transect_groups, const Options &options);

void create_transects_from_baseline(const Path &path, const Path &output_path,
                                    gm::TransectGroups *output_transects,
                                    const Options &options,
                                    const std::string &field_name = "");

void create_intersects_by_transects(gm::TransectGroups &transects,
                                    const Path &shoreline_folders,
                                    const Path &output, const Options &options,
                                    const std::string &proj);

void create_intersects_by_transects(gm::TransectGroups &transects,
                                    const Path &shoreline_shp_path,
                                    const std::string &date_field_name,
                                    const Path &output);

}  // namespace dsas

#endif  // SHORELINECALCULATOR_SHORELINECALCULATOR_HPP
