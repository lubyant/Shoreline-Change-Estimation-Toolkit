#include "shorelinecalculator.hpp"
#include "utility.hpp"

int main() {
  using namespace dsas;
  Options options;
  options.smooth_factor = 10;
  options.transect_spacing = 10;
  options.transect_length = 1000;
  options.edge_distance = 200;
  options.shoreline_least_factor = 0.1;
  options.transect_offset = 0;
  options.transect_orient = Options::TransectOrientation::Left;
  options.intersection_mode = Options::IntersectionMode::Closest;
  options.outlier_metric = Options::OutlierMetric::None;
  const Path baseline_shp_path =
      "/home/lby/Desktop/ShorelineCalculator/DSAS/baseline.shp";
  const Path transect_shp_path =
      "/home/lby/Desktop/ShorelineCalculator/DSAS/transect.shp";
  const Path shoreline_shp_path =
      "/home/lby/Desktop/ShorelineCalculator/DSAS/bluffCrest_TM.shp";
  const std::string baseline_id_field = "DSAS_id";
  const std::string intersect_path =
      "/home/lby/Desktop/ShorelineCalculator/DSAS/crest_intersects.shp";
  gm::TransectGroups transect_groups;
  create_transects_from_baseline(baseline_shp_path, transect_shp_path,
                                 &transect_groups, options, "DSAS_ID");
  create_intersects_by_transects(transect_groups, shoreline_shp_path, "Date_",
                                 intersect_path);
  return 0;
}