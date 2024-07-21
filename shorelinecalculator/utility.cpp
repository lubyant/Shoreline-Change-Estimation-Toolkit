//
// Created by lby on 10/13/23.
//
#include "utility.hpp"

#include <cassert>
#include <limits>
#include <unordered_set>

#define MAX_DOUBLE std::numeric_limits<double>::max()

namespace util {

ThreadPool::ThreadPool(uint32_t num_threads)
    : num_threads_(num_threads), stop_(false) {
  init_workers();
}

void ThreadPool::init_workers() {
  for (uint32_t i = 0; i < num_threads_; i++) {
    workers_.emplace_back([this]() {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(this->queue_mutex_);
          this->condition_.wait(
              lock, [this]() { return this->stop_ || !this->tasks_.empty(); });
          if (this->stop_ && this->tasks_.empty()) {
            return;
          }
          task = std::move(this->tasks_.front());
          tasks_.pop();
        }
        task();
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;
  }
  condition_.notify_all();
  for (auto &worker : workers_) {
    worker.join();
  }
}

void linearRegressRate(const std::vector<gm::IntersectPoint> &intersections,
                       gm::TransectLine &transect,
                       const dsas::Options &options) {
  // if no intersection
  if (intersections.empty()) {
    throw std::runtime_error("It should not empty");
  }

  // if only one intersection
  if (intersections.size() == 1) {
    transect.num_intersect_ = 1;
    transect.change_rate = 0;
    transect.intersect_info_ = std::to_string(intersections[0].year_) + ", 0.";
    return;
  }

  // copy the intersections and sort the vector
  std::vector<gm::IntersectPoint> copy = intersections;
  std::sort(copy.begin(), copy.end(),
            [](const gm::IntersectPoint &a, const gm::IntersectPoint &b) {
              return a.year_ < b.year_;
            });

  // if two intersections
  if (copy.size() == 2) {
    double d_distance = copy[1].distance_to_ref_ - copy[0].distance_to_ref_;
    double d_year = copy[1].year_ - copy[0].year_;
    transect.change_rate = d_distance / d_year;
    transect.num_intersect_ = 2;
    std::stringstream ss;
    ss << copy[0].year_ << ", " << copy[0].distance_to_ref_ << ". "
       << copy[1].year_ << ", " << copy[1].distance_to_ref_ << ". ";
    transect.intersect_info_ = ss.str();
  }

  /*
  if more than two intersections first remove outlier, then compute the
   rates for consecutive years and set the value to transect
  */
  remove_outliers(copy, options);

  // change rate
  std::vector<double> y, x;
  y.push_back(copy[0].distance_to_ref_);
  x.push_back(static_cast<double>(copy[0].year_));
  for (size_t i = 1; i < copy.size(); ++i) {
    int yearChange = copy[i].year_ - copy[i - 1].year_;

    if (yearChange == 0) {
      // Prevent division by zero
      continue;
    }
    x.push_back(copy[i].year_);
    y.push_back(copy[i].distance_to_ref_);
  }
  transect.set_info(x, y);
}

void save_points(const std::vector<gm::IntersectPoint> &shapes,
                 const char *pszProj,
                 const std::filesystem::path &output_path) {
  // Initialize GDAL
  OGRSpatialReference oSRS;
  if (oSRS.importFromWkt(&pszProj) != OGRERR_NONE) {
    throw std::runtime_error("Projection setting fail!");
  }

  // Get the shapefile driver
  GDALDriver *driver =
      GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
  if (driver == nullptr) {
    throw std::runtime_error("Unable to get ESRI Shapefile driver");
  }

  // Create a new shapefile
  GDALDataset *dataset = driver->Create(output_path.string().c_str(), 0, 0, 0,
                                        GDT_Unknown, nullptr);

  // Step 4: Create a layer for the shapefile
  OGRLayer *layer =
      dataset->CreateLayer("pointLayer", &oSRS, wkbPoint, nullptr);
  if (layer == nullptr) {
    throw std::runtime_error("Layer is not created!");
  }

  // define attributes
  for (size_t i = 0; i < shapes[0].get_names().size(); i++) {
    OGRFieldDefn field(shapes[0].get_names()[i].c_str(),
                       shapes[0].get_types()[i]);
    if (layer->CreateField(&field) != OGRERR_NONE) {
      std::cerr << "Failed to create Name field" << std::endl;
      exit(1);
    }
  }

  // Step 6: Create a line geometry and add points to it
  for (const auto &shape : shapes) {
    // Step 5: Create a new feature
    OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
    // Step 7: Add the geometry to the feature
    OGRPoint point;
    point.setX(shape.x);
    point.setY(shape.y);
    feature->SetGeometry(&point);

    set_ogr_feature(shape.get_names(), shape.get_values(), *feature);

    if (layer->CreateFeature(feature) != OGRERR_NONE) {
      std::cerr << "Failed to create feature in shapefile!" << std::endl;
      exit(1);
    }

    OGRFeature::DestroyFeature(feature);
  }

  // Clean up
  GDALClose(dataset);
}

void save_points(const std::vector<gm::TransectLine> &shapes,
                 const char *pszProj,
                 const std::filesystem::path &output_path) {
  // Initialize GDAL
  OGRSpatialReference oSRS;
  if (oSRS.importFromWkt(&pszProj) != OGRERR_NONE) {
    throw std::runtime_error("Projection setting fail!");
  }

  // Get the shapefile driver
  GDALDriver *driver =
      GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
  if (driver == nullptr) {
    throw std::runtime_error("Unable to get ESRI Shapefile driver");
  }

  // Create a new shapefile
  GDALDataset *dataset = driver->Create(output_path.string().c_str(), 0, 0, 0,
                                        GDT_Unknown, nullptr);

  // Step 4: Create a layer for the shapefile
  OGRLayer *layer =
      dataset->CreateLayer("pointLayer", &oSRS, wkbPoint, nullptr);
  if (layer == nullptr) {
    throw std::runtime_error("Layer is not created!");
  }

  // define attributes
  for (size_t i = 0; i < shapes[0].get_names().size(); i++) {
    OGRFieldDefn field(shapes[0].get_names()[i].c_str(),
                       shapes[0].get_types()[i]);
    if (layer->CreateField(&field) != OGRERR_NONE) {
      std::cerr << "Failed to create Name field" << std::endl;
      exit(1);
    }
  }

  // Step 6: Create a line geometry and add points to it
  for (const auto &shape : shapes) {
    // Step 5: Create a new feature
    OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
    // Step 7: Add the geometry to the feature
    OGRPoint point;
    point.setX(shape.transect_base_point_.x);
    point.setY(shape.transect_base_point_.y);
    feature->SetGeometry(&point);

    set_ogr_feature(shape.get_names(), shape.get_values(), *feature);

    if (layer->CreateFeature(feature) != OGRERR_NONE) {
      std::cerr << "Failed to create feature in shapefile!" << std::endl;
      exit(1);
    }

    OGRFeature::DestroyFeature(feature);
  }

  // Clean up
  GDALClose(dataset);
}

double least_square(const std::vector<double> &x,
                    const std::vector<double> &y) {
  if (x.size() != y.size()) {
    throw std::runtime_error("x, y need to have the same size!");
  }
  if (x.empty() || y.empty()) {
    return -999.99;
  }
  double mean_x, mean_y, sum_x = 0, sum_y = 0;
  for (size_t i = 0; i < x.size(); i++) {
    sum_x += x[i];
    sum_y += y[i];
  }
  mean_x = sum_x / static_cast<double>(x.size());
  mean_y = sum_y / static_cast<double>(y.size());

  double var = 0, co_var = 0;
  for (size_t i = 0; i < x.size(); i++) {
    var += (x[i] - mean_x) * (x[i] - mean_x);
    co_var += (x[i] - mean_x) * (y[i] - mean_y);
  }
  if (var == 0) {
    return -999.99;
  }
  return co_var / var;
}

std::string get_tiff_proj(const std::string &path) {
  GDALAllRegister();
  auto *poTIFFDataset =
      static_cast<GDALDataset *>(GDALOpen(path.c_str(), GA_ReadOnly));
  if (poTIFFDataset == nullptr) {
    throw std::runtime_error("Not available tiff!");
  }
  std::string psz_prj_ = std::string(poTIFFDataset->GetProjectionRef());
  GDALClose(poTIFFDataset);
  return psz_prj_;
}

std::string get_shp_proj(const char *path) {
  GDALAllRegister();

  GDALDataset *poDS;
  poDS = static_cast<GDALDataset *>(
      GDALOpenEx(path, GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
  if (poDS == nullptr) {
    throw std::runtime_error("shapefile not open!");
  }

  OGRLayer *poLayer = poDS->GetLayer(0);
  OGRSpatialReference *poSRS = poLayer->GetSpatialRef();
  std::string psz_prj_;
  if (poSRS != nullptr) {
    char *pszProjection;
    poSRS->exportToWkt(&pszProjection);
    psz_prj_ = std::string(pszProjection);
    CPLFree(pszProjection);
  } else {
    throw std::runtime_error("No spatial reference information available\n");
  }
  return psz_prj_;
}

template <>
void save_lines<gm::TransectLine>(const std::vector<gm::TransectLine> &lines,
                                  const char *pszProj,
                                  const std::filesystem::path &output_path) {
  GDALAllRegister();
  OGRSpatialReference oSRS;
  if (oSRS.importFromWkt(&pszProj) != OGRERR_NONE) {
    throw std::runtime_error("Projection setting fail!");
  }

  GDALDriver *driver =
      GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");

  GDALDataset *dataset = driver->Create(output_path.string().c_str(), 0, 0, 0,
                                        GDT_Unknown, nullptr);
  if (!dataset) {
    throw std::runtime_error("Failed to create dataset");
  }

  OGRLayer *layer = dataset->CreateLayer("line", &oSRS, wkbLineString, nullptr);
  if (!layer) {
    throw std::runtime_error("Failed to create layer");
  }

  OGRFieldDefn baseline_id("BaselineId", OFTInteger);
  if (layer->CreateField(&baseline_id) != OGRERR_NONE) {
    std::cerr << "Failed to create Name field" << std::endl;
    exit(1);
  }
  OGRFieldDefn transect_id("TransectId", OFTInteger);
  if (layer->CreateField(&transect_id) != OGRERR_NONE) {
    std::cerr << "Failed to create Name field" << std::endl;
    exit(1);
  }
  OGRFieldDefn image_id("ImageId", OFTInteger);
  if (layer->CreateField(&image_id) != OGRERR_NONE) {
    std::cerr << "Failed to create Name field" << std::endl;
    exit(1);
  }
  OGRFieldDefn change_rate("ChangeRate", OFTReal);
  change_rate.SetWidth(8);
  change_rate.SetPrecision(3);
  if (layer->CreateField(&change_rate) != OGRERR_NONE) {
    std::cerr << "Failed to create Name field" << std::endl;
    exit(1);
  }

  for (const auto &shape : lines) {
    OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
    if (!feature) {
      throw std::runtime_error("Failed to create feature");
    }
    OGRLineString line;

    // Step 6: Create a line geometry and add points to it
    for (size_t i = 0; i < shape.size(); i++) {
      line.addPoint(shape[i].x, shape[i].y);
    }

    // Step 7: Add the geometry to the feature
    auto err = feature->SetGeometry(&line);
    if (err != OGRERR_NONE) {
      throw std::runtime_error("Failed to set geometry");
    }
    feature->SetField("BaselineId", shape.baseline_id_);
    feature->SetField("TransectId", shape.transect_id_);
    feature->SetField("ImageId", shape.image_id_);
    feature->SetField("ChangeRate", shape.change_rate);

    // Step 8: Add the feature to the layer
    err = layer->CreateFeature(feature);
    if (err != OGRERR_NONE) {
      throw std::runtime_error("Failed to set geometry");
    }
    OGRFeature::DestroyFeature(feature);
  }

  // Clean up
  GDALClose(dataset);
}

void remove_outliers(std::vector<double> &x, std::vector<double> &y,
                     double threshold) {
  // check dimension
  size_t nx = x.size(), ny = y.size();
  if (nx != ny) {
    throw std::runtime_error("x, y is not in the same size!");
  }

  // compute the standard deviation
  double standard_dev;
  double mean;
  mean =
      std::accumulate(y.begin(), y.end(), 0.0) / static_cast<double>(y.size());
  standard_dev =
      std::sqrt(std::accumulate(y.begin(), y.end(), 0.0,
                                [mean](double sum, double val) {
                                  return sum + (val - mean) * (val - mean);
                                }) /
                static_cast<double>(y.size() - 1));

  // remove outlier
  for (size_t i = 0; i < y.size(); i++) {
    if (std::abs(y[i] - mean) > threshold * standard_dev) {
      x.erase(x.begin() + static_cast<std::vector<double>::difference_type>(i));
      y.erase(y.begin() + static_cast<std::vector<double>::difference_type>(i));
      i--;
    }
  }
}

void remove_outliers(std::vector<gm::IntersectPoint> &intersects,
                     const dsas::Options &options) {
  double standard_dev;
  double mean;
  switch (options.outlier_metric) {
    case dsas::Options::OutlierMetric::None:
      return;
    case dsas::Options::OutlierMetric::FrechetDistance:
      // compute the standard deviation
      mean =
          std::accumulate(intersects.begin(), intersects.end(), 0.0,
                          [](int pre_sum, const gm::IntersectPoint &intersect) {
                            return pre_sum + intersect.frechet_distance_diff_;
                          }) /
          static_cast<double>(intersects.size());
      standard_dev = std::sqrt(
          std::accumulate(
              intersects.begin(), intersects.end(), 0.0,
              [mean](double pre_sum, const gm::IntersectPoint &intersect) {
                return pre_sum + (intersect.frechet_distance_diff_ - mean) *
                                     (intersect.frechet_distance_diff_ - mean);
              }) /
          static_cast<double>(intersects.size() - 1));
      // remove outlier
      for (size_t i = 0; i < intersects.size(); i++) {
        if (std::abs(intersects[i].frechet_distance_diff_ - mean) >
            options.outlier_rate * standard_dev) {
          intersects.erase(intersects.begin() + i);
          i--;
        }
      }
      break;
    case dsas::Options::OutlierMetric::BaseDistance:
      // remove the outliers based on base distance
      mean =
          std::accumulate(intersects.begin(), intersects.end(), 0.0,
                          [](int pre_sum, const gm::IntersectPoint &intersect) {
                            return pre_sum + intersect.distance_to_ref_;
                          }) /
          static_cast<double>(intersects.size());
      standard_dev = std::sqrt(
          std::accumulate(
              intersects.begin(), intersects.end(), 0.0,
              [mean](double pre_sum, const gm::IntersectPoint &intersect) {
                return pre_sum + (intersect.distance_to_ref_ - mean) *
                                     (intersect.distance_to_ref_ - mean);
              }) /
          static_cast<double>(intersects.size() - 1));
      // remove outlier
      for (size_t i = 0; i < intersects.size(); i++) {
        if (std::abs(intersects[i].distance_to_ref_ - mean) >
            options.outlier_rate * standard_dev) {
          intersects.erase(intersects.begin() + i);
          i--;
        }
      }
      break;
    default:
      std::cerr << __FILE__;
      throw std::runtime_error(": not a valid metric");
  }
}

gm::Baselines load_baselines_shp(const gm::Path &baseline_shp_path,
                                 const std::string &field_name,
                                 const dsas::Options &options) {
  // Initialize GDAL
  GDALAllRegister();

  // Open the Shapefile
  GDALDataset *poDS;
  poDS = static_cast<GDALDataset *>(GDALOpenEx(
      baseline_shp_path.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
  if (poDS == nullptr) {
    std::cerr << "Open failed.\n";
    exit(1);
  }

  // Get the Layer Containing the Line Features
  OGRLayer *poLayer;
  poLayer = poDS->GetLayer(0);

  // Iterate Through the Features in the Layer and Access Points
  OGRFeature *poFeature;
  poLayer->ResetReading();
  gm::Baselines baselines;
  while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry *poGeometry;
    poGeometry = poFeature->GetGeometryRef();
    if (poGeometry != nullptr &&
        wkbFlatten(poGeometry->getGeometryType()) == wkbLineString) {
      auto *poLine = dynamic_cast<OGRLineString *>(poGeometry);
      int numPoints = poLine->getNumPoints();

      std::vector<gm::Point<double>> baseline_vertices;
      for (int i = 0; i < numPoints; i++) {
        OGRPoint point;
        poLine->getPoint(i, &point);
        baseline_vertices.emplace_back(point.getX(), point.getY());
      }
      int baseline_id{poFeature->GetFieldAsInteger(field_name.c_str())};
      gm::Baseline baseline{baseline_vertices, baseline_id, baseline_id,
                            options};
      baselines.push_back(std::move(baseline));
    } else {
      std::cout << "No geometry\n";
    }
    OGRFeature::DestroyFeature(poFeature);
  }

  // Cleanup
  GDALClose(poDS);

  return baselines;
}
gm::Shorelines load_shorelines_shp(const gm::Path &shoreline_shp_path,
                                   const std::string &baseline_proj,
                                   int image_id) {
  gm::Shorelines shorelines;
  int shoreline_id{0};

  // Initialize GDAL
  GDALAllRegister();

  // Open the Shapefile
  GDALDataset *poDS;
  poDS = static_cast<GDALDataset *>(GDALOpenEx(
      shoreline_shp_path.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
  if (poDS == nullptr) {
    std::cerr << "Open failed.\n";
    exit(1);
  }

  // get coordiante system
  const char *pszRefProj = baseline_proj.c_str();
  OGRSpatialReference refSRS;
  if (refSRS.importFromWkt(pszRefProj) != OGRERR_NONE) {
    throw std::runtime_error("Failed to import reference spatial reference.\n");
  }

  // Get the Layer Containing the Line Features
  OGRLayer *poLayer;
  poLayer = poDS->GetLayer(0);
  OGRSpatialReference *pszInputProj = poLayer->GetSpatialRef();
  OGRCoordinateTransformation *coordTransform;
  coordTransform = OGRCreateCoordinateTransformation(pszInputProj, &refSRS);
  if (coordTransform == nullptr) {
    throw std::runtime_error("Failed to create coordinate transformation.\n");
  }

  // Iterate Through the Features in the Layer and Access Points
  OGRFeature *poFeature;
  poLayer->ResetReading();
  gm::Baselines baselines;
  while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry *poGeometry;
    poGeometry = poFeature->GetGeometryRef();
    // Transform the geometry
    if (poGeometry->transform(coordTransform) != OGRERR_NONE) {
      throw std::runtime_error("fail to transform!");
    }
    if (poGeometry != nullptr &&
        wkbFlatten(poGeometry->getGeometryType()) == wkbLineString) {
      auto *poLine = dynamic_cast<OGRLineString *>(poGeometry);
      int numPoints = poLine->getNumPoints();

      std::vector<gm::Point<double>> shoreline_vertices;
      for (int i = 0; i < numPoints; i++) {
        OGRPoint point;
        poLine->getPoint(i, &point);
        shoreline_vertices.emplace_back(point.getX(), point.getY());
      }
      int year{poFeature->GetFieldAsInteger("year")};
      shorelines.emplace_back(shoreline_vertices, shoreline_id++, year,
                              image_id);
    } else {
      std::cout << "No geometry\n";
    }
    OGRFeature::DestroyFeature(poFeature);
  }

  // Cleanup
  GDALClose(poDS);

  return shorelines;
}
gm::Shorelines load_shorelines_shp(const gm::Path &shoreline_shp_path,
                                   const std::string &baseline_proj) {
  gm::Shorelines shorelines;
  int shoreline_id{0};

  // Initialize GDAL
  GDALAllRegister();

  // Open the Shapefile
  GDALDataset *poDS;
  poDS = static_cast<GDALDataset *>(GDALOpenEx(
      shoreline_shp_path.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
  if (poDS == nullptr) {
    std::cerr << "Open failed.\n";
    exit(1);
  }

  // get coordiante system
  const char *pszRefProj = baseline_proj.c_str();
  OGRSpatialReference refSRS;
  if (refSRS.importFromWkt(pszRefProj) != OGRERR_NONE) {
    throw std::runtime_error("Failed to import reference spatial reference.\n");
  }

  // Get the Layer Containing the Line Features
  OGRLayer *poLayer;
  poLayer = poDS->GetLayer(0);
  OGRSpatialReference *pszInputProj = poLayer->GetSpatialRef();
  OGRCoordinateTransformation *coordTransform;
  coordTransform = OGRCreateCoordinateTransformation(pszInputProj, &refSRS);
  if (coordTransform == nullptr) {
    throw std::runtime_error("Failed to create coordinate transformation.\n");
  }

  // Iterate Through the Features in the Layer and Access Points
  OGRFeature *poFeature;
  poLayer->ResetReading();
  gm::Baselines baselines;
  while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry *poGeometry;
    poGeometry = poFeature->GetGeometryRef();
    // Transform the geometry
    if (poGeometry->transform(coordTransform) != OGRERR_NONE) {
      throw std::runtime_error("fail to transform!");
    }
    if (poGeometry != nullptr &&
        wkbFlatten(poGeometry->getGeometryType()) == wkbLineString) {
      auto *poLine = dynamic_cast<OGRLineString *>(poGeometry);
      int numPoints = poLine->getNumPoints();

      std::vector<gm::Point<double>> shoreline_vertices;
      for (int i = 0; i < numPoints; i++) {
        OGRPoint point;
        poLine->getPoint(i, &point);
        shoreline_vertices.emplace_back(point.getX(), point.getY());
      }
      std::string date = std::string(poFeature->GetFieldAsString("Date_"));
      int year = std::atoi(date.substr(6, 4).c_str());
      int image_id{poFeature->GetFieldAsInteger("ImageID")};
      shorelines.emplace_back(shoreline_vertices, shoreline_id++, year,
                              image_id);
    } else {
      std::cout << "No geometry\n";
    }
    OGRFeature::DestroyFeature(poFeature);
  }

  // Cleanup
  GDALClose(poDS);

  return shorelines;
}
void remove_same_year_intersections(
    std::vector<gm::IntersectPoint> &intersect_points,
    const dsas::Options::IntersectionMode &mode) {
  struct DateHash {
    std::size_t operator()(const boost::gregorian::date &d) const {
      constexpr std::hash<int> int_hash;
      return int_hash(d.year()) ^ int_hash(d.month()) ^ int_hash(d.day());
    }
  };
  struct DateEqual {
    bool operator()(const boost::gregorian::date &d1,
                    const boost::gregorian::date &d2) const {
      return d1 == d2;
    }
  };
  std::unordered_map<boost::gregorian::date, std::vector<gm::IntersectPoint>,
                     DateHash, DateEqual>
      avail_dates;
  for (const auto &point : intersect_points) {
    avail_dates[point.date_].push_back(point);
  }
  std::vector<gm::IntersectPoint> new_intersects;
  for (auto &[date, points] : avail_dates) {
    auto target_point = std::max_element(
        points.begin(), points.end(),
        [&](const gm::IntersectPoint &a, const gm::IntersectPoint &b) {
          if (mode == dsas::Options::IntersectionMode::Closest) {
            return a.distance_to_ref_ >= b.distance_to_ref_;
          }
          return a.distance_to_ref_ < b.distance_to_ref_;
        });
    new_intersects.push_back(std::move(*target_point));
  }
  intersect_points = std::move(new_intersects);
}
std::vector<gm::Point<>> get_subset_of_vertices(
    const std::vector<gm::Point<>> &line, const gm::Point<> &p1,
    const gm::Point<> &p2) {
  auto isBefore = [](const gm::Point<> &a, const gm::Point<> &b) {
    return a.x < b.x || (a.x == b.x && a.y < b.y);
  };

  gm::Point<> before_p1, after_p1, before_p2, after_p2;

  double min_dist_before_p1{MAX_DOUBLE}, min_dist_after_p1{MAX_DOUBLE},
      min_dist_before_p2{MAX_DOUBLE}, min_dist_after_p2{MAX_DOUBLE};
  for (const auto &vertex : line) {
    if (isBefore(vertex, p1)) {
      auto dist = vertex.distance_to_point((p1));
      if (dist < min_dist_before_p1) {
        before_p1 = vertex;
        min_dist_before_p1 = dist;
      }
    } else {
      auto dist = vertex.distance_to_point((p1));
      if (dist < min_dist_after_p1) {
        after_p1 = vertex;
        min_dist_after_p1 = dist;
      }
    }

    if (isBefore(vertex, p2)) {
      auto dist = vertex.distance_to_point((p2));
      if (dist < min_dist_before_p2) {
        before_p2 = vertex;
        min_dist_before_p2 = dist;
      }
    } else {
      auto dist = vertex.distance_to_point((p2));
      if (dist < min_dist_after_p2) {
        after_p2 = vertex;
        min_dist_after_p2 = dist;
      }
    }
  }

  auto it1 = std::find(line.begin(), line.end(), before_p1);
  auto it2 = std::find(line.begin(), line.end(), after_p1);
  auto it3 = std::find(line.begin(), line.end(), before_p2);
  auto it4 = std::find(line.begin(), line.end(), after_p2);

  if (isBefore(p1, p2)) {
    if (it2 < it3) {
      return {it2, it3 + 1};
    }
    return {it3, it2 + 1};
  }

  if (it4 < it1) {
    return {it4, it1 + 1};
  }
  return {it1, it4 + 1};
}
std::optional<gm::Shoreline> truncate_shore_by_transects(
    const gm::TransectLine &tran1, const gm::TransectLine &tran2,
    const gm::Shoreline &shoreline) {
  auto vertices = shoreline.shoreline_vertices_;
  auto year = shoreline.year_;
  auto year_intersect_map1 = tran1.year_intersect_map_;
  auto year_intersect_map2 = tran2.year_intersect_map_;
  if (year_intersect_map1.find(year) != year_intersect_map1.end() &&
      year_intersect_map2.find(year) != year_intersect_map2.end()) {
    gm::Shoreline ret_shoreline = shoreline;
    auto intersect1 = year_intersect_map1[year];
    auto intersect2 = year_intersect_map2[year];
    ret_shoreline.shoreline_vertices_ =
        std::move(get_subset_of_vertices(vertices, *intersect1, *intersect2));
    return ret_shoreline;
  }
  return std::nullopt;
}

std::optional<gm::Shoreline> truncate_shore_by_intersect(
    const gm::IntersectPoint &intersect) {
  auto *transect_line = intersect.transect_line_ptr_;
  assert(transect_line->prev_transect_line != nullptr ||
         transect_line->next_transect_line != nullptr);
  const auto year = intersect.year_;

  auto *prev_transect{transect_line}, *next_transect{transect_line};

  if (transect_line->prev_transect_line != nullptr) {
    prev_transect = transect_line->prev_transect_line;
  }
  if (transect_line->next_transect_line != nullptr) {
    next_transect = transect_line->next_transect_line;
  }
  if (prev_transect->year_intersect_map_.find(year) ==
          prev_transect->year_intersect_map_.end() ||
      next_transect->year_intersect_map_.find(year) ==
          next_transect->year_intersect_map_.end()) {
    return std::nullopt;
  }
  const gm::IntersectPoint *prev_intersect =
      prev_transect->year_intersect_map_.find(year)->second;
  const gm::IntersectPoint *next_intersect =
      (next_transect->year_intersect_map_).find(year)->second;
  if (prev_intersect->shoreline_ptr_ != next_intersect->shoreline_ptr_) {
    return std::nullopt;
  }
  gm::Shoreline truncate_shoreline = *(prev_intersect->shoreline_ptr_);
  auto vertices = prev_intersect->shoreline_ptr_->shoreline_vertices_;
  truncate_shoreline.shoreline_vertices_ = std::move(
      get_subset_of_vertices(vertices, *prev_intersect, *next_intersect));
  return truncate_shoreline;
}

double frechet_distance(std::vector<gm::Point<>> line1,
                        std::vector<gm::Point<>> line2) {
  size_t m = line1.size();
  size_t n = line2.size();
  std::vector<std::vector<double>> D(m, std::vector<double>(n, 0.0));
  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < n; ++j) {
      D[i][j] = line1[i].distance_to_point(line2[j]);
    }
  }
  // Initialize matrix F
  std::vector<std::vector<double>> F(m, std::vector<double>(n, -1.0));
  F[0][0] = D[0][0];
  // Initialize first row and first column of F
  for (int i = 1; i < m; ++i) {
    F[i][0] = std::max(F[i - 1][0], D[i][0]);
  }
  for (int j = 1; j < n; ++j) {
    F[0][j] = std::max(F[0][j - 1], D[0][j]);
  }

  // Fill in the rest of F
  for (int i = 1; i < m; ++i) {
    for (int j = 1; j < n; ++j) {
      F[i][j] = std::max(std::min({F[i - 1][j], F[i - 1][j - 1], F[i][j - 1]}),
                         D[i][j]);
    }
  }

  return F[m - 1][n - 1];
}
std::vector<gm::Shoreline> util::truncate_shore_by_transects(
    const gm::TransectLine &tran1, const gm::TransectLine &tran2) {
  std::vector<gm::Shoreline> shorelines_segs;
  auto year_intersect_map1 = tran1.year_intersect_map_;
  auto year_intersect_map2 = tran2.year_intersect_map_;

  std::vector<int> common_years;
  for (const auto pair : year_intersect_map1) {
    if (year_intersect_map2.find(pair.first) != year_intersect_map2.end()) {
      common_years.push_back((pair.first));
    }
  }

  if (common_years.empty()) {
    return {};
  }

  for (int year : common_years) {
    auto *intersect1 = year_intersect_map1[year];
    auto *intersect2 = year_intersect_map2[year];
    auto *shoreline1 = intersect1->shoreline_ptr_;
    auto *shoreline2 = intersect2->shoreline_ptr_;
    if (shoreline1 != shoreline2) {
      continue;
    }
    auto sub_vertices = get_subset_of_vertices(shoreline1->shoreline_vertices_,
                                               *intersect1, *intersect2);
    if (sub_vertices.empty()) {
      continue;
    }
    shorelines_segs.emplace_back(sub_vertices, shoreline1->shoreline_id_, year,
                                 shoreline1->image_id_);
  }

  return shorelines_segs;
}
}  // namespace util