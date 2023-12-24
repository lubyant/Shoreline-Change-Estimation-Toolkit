//
// Created by lby on 10/13/23.
//
#include "utility.h"

#include <functional>
#include <iostream>
#include <tuple>

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

double linearRegressRate(const std::vector<gm::IntersectPoint> &intersections) {
  // if no intersection
  if (intersections.empty()) {
    throw std::runtime_error("It should not empty");
  }

  // if only one intersection
  if (intersections.size() == 1) {
    return 0;
  }

  // sort the vector
  std::vector<gm::IntersectPoint> copy = intersections;
  std::sort(copy.begin(), copy.end(),
            [](const gm::IntersectPoint &a, const gm::IntersectPoint &b) {
              return a.year_ < b.year_;
            });

  // if two intersections
  if (copy.size() == 2) {
    double d_distance = copy[1].distance_to_ref_ - copy[0].distance_to_ref_;
    double d_year = copy[1].year_ - copy[0].year_;
    return d_distance / d_year;
  }

  // if more than two intersections
  // Compute the rates for consecutive years
  std::vector<double> y;
  std::vector<double> x;
  for (size_t i = 1; i < copy.size(); ++i) {
    int yearChange = copy[i].year_ - copy[i - 1].year_;

    if (yearChange == 0) {
      // Prevent division by zero
      continue;
    }

    x.push_back(copy[i].year_);
    y.push_back(copy[i].distance_to_ref_);
  }
  return least_square(x, y);
}
void save_points(const std::vector<gm::IntersectPoint> &shapes,
                 const char *pszProj,
                 const std::filesystem::path &output_path) {
  // Initialize GDAL
  GDALAllRegister();
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
double least_square(std::vector<double> &x, std::vector<double> &y) {
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
template <>
void save_lines<gm::TransectLine>(std::vector<gm::TransectLine> &lines,
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

}  // namespace util