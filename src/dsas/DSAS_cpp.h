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
  std::vector<gm::TransectLine> transects_;
};

using Path = std::filesystem::path;
using Baselines = std::vector<gm::Baseline>;
using TransectGroups = std::vector<Transects>;

void digital_shoreline_analysis_system(const Path &folder,
                                       const Path &output_path);

void digital_shoreline_analysis_system(const std::vector<Path> &paths,
                                       const Path &output_path);

void controller(const std::vector<Path> &paths, const Path &output_path);

Baselines generate_baselines(const std::vector<std::unique_ptr<Image>> &images);

TransectGroups generate_transects(const Baselines &baselines);
}  // namespace dsas

#endif  // DSAS_CPP_DSAS_CPP_H
