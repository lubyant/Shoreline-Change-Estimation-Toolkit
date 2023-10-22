//
// Created by lby on 10/12/23.
//

#ifndef DSAS_CPP_DSAS_CPP_H
#define DSAS_CPP_DSAS_CPP_H
#include <filesystem>
#include <vector>

#include "image.h"
#include "geometry.h"
namespace dsas{
    using Path = std::filesystem::path ;

    void digital_shoreline_analysis_system(const Path& folder, const Path& output_path);

    void digital_shoreline_analysis_system(const std::vector<Path>& paths, const Path& output_path);

    void controller(const std::vector<Path> &paths, const Path &output_path);

    std::vector<gm::Baseline> generate_baseline(const std::vector<std::unique_ptr<Image>> &images);
}

#endif //DSAS_CPP_DSAS_CPP_H
