//
// Created by lby on 10/12/23.
//

#ifndef SHORELINECALCULATOR_SHORELINECALCULATOR_HPP
#define SHORELINECALCULATOR_SHORELINECALCULATOR_HPP

#include <filesystem>
#include <iterator>
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
    const std::vector<Image> &images, const TransectGroups &TransectGroups);

umap<int, umap<int, std::vector<gm::IntersectPoint>>> generate_intersections(
    const Shorelines &shorelines, TransectGroups &transect_groups);

std::vector<gm::IntersectPoint> generate_intersection(
    const Shorelines &shorelines, TransectGroups &transect_groups);

void compute_rate(const umap<int, umap<int, std::vector<gm::IntersectPoint>>>
                      &intersection_maps,
                  TransectGroups &transect_groups, const Options &options);

void create_transects_from_baseline(const Path &path, const Path &output_path,
                                    TransectGroups *output_transects,
                                    const Options &options);

void create_intersects_by_transects(TransectGroups &transects,
                                    const Path &shoreline_folders,
                                    const Path &output, const Options &options,
                                    const std::string &proj);

class ShoresIterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = umap<int, std::vector<Point<double>> *>;
  using pointer = value_type *;
  using reference = value_type &;
  ShoresIterator(const gm::Shorelines &shorelines,
                 const gm::TransectGroups &transect_groups, size_t pos)
      : shorelines_(shorelines), transect_groups_(transect_groups) {}

  value_type operator*() const;

 private:
  const gm::Shorelines &shorelines_;
  const gm::TransectGroups &transect_groups_;
  size_t baseline_pos_ = 0;
  size_t transect_pos_ = 0;
};

}  // namespace dsas

#endif  // SHORELINECALCULATOR_SHORELINECALCULATOR_HPP
