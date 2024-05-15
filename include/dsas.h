//
// Created by lby on 10/26/23.
//

#ifndef DSAS_CPP_INCLUDE_DSAS_H_
#define DSAS_CPP_INCLUDE_DSAS_H_

#include "../src/dsas/DSAS_cpp.h"
#include "../src/dsas/options.h"
namespace dsas {
using Path = std::filesystem::path;

void dsas(const std::vector<Path> &folders, const Path &output_path,
          const Options &options);

void digital_shoreline_analysis_system(const Path &folder,
                                       const Path &output_path,
                                       const Options &options);

void digital_shoreline_analysis_system(const std::vector<Path> &paths,
                                       const Path &output_path,
                                       const Options &options);

void create_transects_from_baseline(const Path &path, const Path &output_path,
                                    const Options &options);

}  // namespace dsas
#endif  // DSAS_CPP_INCLUDE_DSAS_H_
