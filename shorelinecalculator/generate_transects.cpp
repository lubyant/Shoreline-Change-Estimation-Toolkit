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
      "/home/lby/Desktop/ShorelineCalculator/new_indiana/baseline.shp";
  const Path transect_shp_path =
      "/home/lby/Desktop/ShorelineCalculator/new_indiana/transect.shp";
  const Path shoreline_shp_path =
      "/home/lby/Desktop/ShorelineCalculator/new_indiana/new_indiana_prj.shp";
  const std::string baseline_id_field = "Id";
  const std::string intersect_path =
      "/home/lby/Desktop/ShorelineCalculator/new_indiana/intersects.shp";
  const std::string result_path{
      "/home/lby/Desktop/ShorelineCalculator/new_indiana/results.shp"};
  gm::TransectGroups transect_groups;
  create_transects_from_baseline(baseline_shp_path, transect_shp_path,
                                 &transect_groups, options, baseline_id_field);
  auto intersections = create_intersects_by_transects(
      transect_groups, shoreline_shp_path, "Date_", intersect_path);
  calculate_erosion_rate(intersections, transect_groups, result_path, options);
  return 0;
}