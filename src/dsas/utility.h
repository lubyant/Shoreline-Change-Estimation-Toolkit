//
// Created by lby on 10/13/23.
//

#ifndef DSAS_CPP_UTILITY_H
#define DSAS_CPP_UTILITY_H

#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <numeric>
#include <queue>
#include <tuple>
#include <vector>

#include "geometry.h"
#include "options.h"

#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

#ifndef MAX
#define MAX(A, B) ((A) < (B) ? (B) : (A))
#endif

#ifndef RMS
#define RMS(x, y) sqrt((x) * (x) + (y) * (y))
#endif

namespace util {

template <typename T>
void save_lines(std::vector<T> &lines, std::filesystem::path &output_path);

template <typename T>
T crossProduct(std::vector<T> &vec1, std::vector<T> &vec2) {
  return vec1[0] * vec2[1] - vec1[1] * vec2[0];
}

template <typename T>
T calCrossOfTwoVectors(const gm::Point<T> &p1, const gm::Point<T> &p2,
                       const gm::Point<T> &p3, const gm::Point<T> &p4) {
  T x1 = p2.x - p1.x;
  T x2 = p4.x - p3.x;
  T y1 = p2.y - p1.y;
  T y2 = p4.y - p3.y;
  std::vector<T> v1 = {x1, y1};
  std::vector<T> v2 = {x2, y2};

  return crossProduct(v1, v2);
}

template <typename T>
bool testRectangularOfIntersection(const gm::Point<T> &p1,
                                   const gm::Point<T> &p2,
                                   const gm::Point<T> &p3,
                                   const gm::Point<T> &p4) {
  T l_x_min = MIN(p1.x, p2.x);
  T l_x_max = MAX(p1.x, p2.x);
  T r_x_min = MIN(p3.x, p4.x);
  T r_x_max = MAX(p3.x, p4.x);
  T l_y_min = MIN(p1.y, p2.y);
  T l_y_max = MAX(p1.y, p2.y);
  T r_y_min = MIN(p3.y, p4.y);
  T r_y_max = MAX(p3.y, p4.y);
  return (l_x_max >= r_x_min) && (r_x_max >= l_x_min) && (r_y_max >= l_y_min) &&
         (l_y_max >= r_y_min);
}

template <typename T>
bool isTwoSegmentIntersected(const gm::Point<T> &p1, const gm::Point<T> &p2,
                             const gm::Point<T> &p3, const gm::Point<T> &p4) {
  if (testRectangularOfIntersection(p1, p2, p3, p4)) {
    if ((calCrossOfTwoVectors(p3, p1, p3, p4) *
             calCrossOfTwoVectors(p3, p2, p3, p4) <=
         0) &&
        (calCrossOfTwoVectors(p2, p3, p2, p1) *
             calCrossOfTwoVectors(p2, p4, p2, p1) <=
         0)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

void linearRegressRate(const std::vector<gm::IntersectPoint> &intersections,
                       gm::TransectLine &transect, double outlier_rate);

template <typename T>
gm::Point<T> computeIntersectPoint(const gm::Point<T> &p1,
                                   const gm::Point<T> &p2,
                                   const gm::Point<T> &p3,
                                   const gm::Point<T> &p4) {
  double x1 = p1.x, y1 = p1.y;
  double x2 = p2.x, y2 = p2.y;
  double x3 = p3.x, y3 = p3.y;
  double x4 = p4.x, y4 = p4.y;
  double a0 = y1 - y2, b0 = x2 - x1, c0 = x1 * y2 - x2 * y1;
  double a1 = y3 - y4, b1 = x4 - x3, c1 = x3 * y4 - x4 * y3;
  double d = a0 * b1 - a1 * b0;
  if (d == 0)
    return {-999999, -999999};
  else {
    double x = (b0 * c1 - b1 * c0) / d;
    double y = (c0 * a1 - c1 * a0) / d;
    return {x, y};
  }
}

class ThreadPool {
 public:
  explicit ThreadPool(
      uint32_t num_threads = std::thread::hardware_concurrency());

  ~ThreadPool();

  template <class F, class... Args>
  auto enqueue(F &&f, Args &&...args)
      -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_t = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_t()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<return_t> res = task->get_future();

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (stop_) {
        throw std::runtime_error("enqueue a stopped threadpool.");
      }
      tasks_.emplace([task] { (*task)(); });
    }

    condition_.notify_one();
    return res;
  }

 private:
  uint32_t num_threads_;
  bool stop_;

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex queue_mutex_;
  std::condition_variable condition_;

  void init_workers();
};

template <size_t I = 0, typename... Args>
typename std::enable_if<I == sizeof...(Args), void>::type set_ogr_feature(
    const std::vector<std::string> &, const std::tuple<Args...> &,
    OGRFeature &) {}
template <size_t I = 0, typename... Args>
typename std::enable_if<I != sizeof...(Args), void>::type set_ogr_feature(
    const std::vector<std::string> &names, const std::tuple<Args...> &values,
    OGRFeature &ogr_feature) {
  ogr_feature.SetField(names[I].c_str(), std::get<I>(values));
  set_ogr_feature<I + 1, Args...>(names, values, ogr_feature);
}

void save_points(const std::vector<gm::IntersectPoint> &shapes,
                 const char *pszProj, const std::filesystem::path &output_path);

void save_points(const std::vector<gm::TransectLine> &shapes,
                 const char *pszProj, const std::filesystem::path &output_path);

template <typename T>
void save_lines(std::vector<T> &lines, const char *pszProj,
                const std::filesystem::path &output_path) {
  // Step 1: Initialize GDAL
  OGRSpatialReference oSRS;
  if (oSRS.importFromWkt(&pszProj) != OGRERR_NONE) {
    throw std::runtime_error("Projection setting fail!");
  }

  // Step 2: Get the shapefile driver
  GDALDriver *driver =
      GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");

  // Step 3: Create a new shapefile
  GDALDataset *dataset = driver->Create(output_path.string().c_str(), 0, 0, 0,
                                        GDT_Unknown, nullptr);
  if (!dataset) {
    throw std::runtime_error("Failed to create dataset");
  }

  // Step 4: Create a layer for the shapefile
  OGRLayer *layer = dataset->CreateLayer("line", &oSRS, wkbLineString, nullptr);
  if (layer == nullptr) {
    throw std::runtime_error("Failed to create layer");
  }

  // define attributes
  for (size_t i = 0; i < lines[0].get_names().size(); i++) {
    OGRFieldDefn field(lines[0].get_names()[i].c_str(),
                       lines[0].get_types()[i]);
    if (layer->CreateField(&field) != OGRERR_NONE) {
      std::cerr << "Failed to create Name field" << std::endl;
      exit(1);
    }
  }

  for (const auto &shape : lines) {
    // Step 5: Create a new feature
    OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
    if (!feature) {
      throw std::runtime_error("Failed to create feature");
    }
    OGRLineString line;

    // Step 6: Create a line geometry and add points to it
    for (size_t i = 0; i < shape.size(); i++) {
      line.addPoint(shape[i].x, shape[i].y);
    }

    set_ogr_feature(shape.get_names(), shape.get_values(), *feature);

    // Step 7: Add the geometry to the feature
    auto err = feature->SetGeometry(&line);
    if (err != OGRERR_NONE) {
      throw std::runtime_error("Failed to set geometry");
    }

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
template <>
void save_lines<gm::TransectLine>(std::vector<gm::TransectLine> &lines,
                                  const char *pszProj,
                                  const std::filesystem::path &output_path);

double least_square(const std::vector<double> &x, const std::vector<double> &y);

std::string get_tiff_proj(const char *path);
std::string get_shp_proj(const char *path);

void remove_outliers(std::vector<double> &x, std::vector<double> &y,
                     double threshold);

gm::Baselines load_baselines_shp(const gm::Path &baseline_shp_path,
                                 const std::string &field_name,
                                 const dsas::Options &options);

gm::Shorelines load_shorelines_shp(const gm::Path &shoreline_shp_path,
                                   const std::string &baseline_proj,
                                   int image_id);

gm::Shorelines load_shorelines_shp(const gm::Path &shoreline_shp_path,
                                   const std::string &baseline_proj);
}  // namespace util
#endif  // DSAS_CPP_UTILITY_H
