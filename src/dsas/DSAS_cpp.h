//
// Created by lby on 10/12/23.
//

#ifndef DSAS_CPP_DSAS_CPP_H
#define DSAS_CPP_DSAS_CPP_H

#include <filesystem>
#include <vector>

#include "geometry.h"
#include "image.h"

namespace dsas {
struct Transects {
  int baseline_id_;
  int image_id_;
  std::vector<gm::TransectLine> transects_;
};

using Path = std::filesystem::path;
using Baselines = std::vector<gm::Baseline>;
using TransectGroups = std::vector<Transects>;

template <typename Key, typename Value>
using umap = std::unordered_map<Key, Value>;

void controller(const std::vector<Path> &paths, const Path &output_path,
                const Options &options);

Baselines generate_baselines(const std::vector<std::unique_ptr<Image>> &images,
                             const Options &options);

TransectGroups generate_transects(const Baselines &baselines);

umap<int, umap<int, std::vector<gm::IntersectPoint>>> generate_intersections(
    const std::vector<std::unique_ptr<Image>> &images,
    const TransectGroups &TransectGroups);

std::vector<gm::IntersectPoint> generate_intersection(
    const std::vector<gm::Shoreline> &shorelines,
    const TransectGroups &transect_groups);

void compute_rate(const umap<int, umap<int, std::vector<gm::IntersectPoint>>>
                      &intersections_maps,
                  TransectGroups &transect_groups);

}  // namespace dsas

#endif  // DSAS_CPP_DSAS_CPP_H
