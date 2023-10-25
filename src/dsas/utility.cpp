//
// Created by lby on 10/13/23.
//
#include "utility.h"

#include <functional>
#include <iostream>

namespace util {

ThreadPool::ThreadPool()
    : num_threads_(std::thread::hardware_concurrency()), stop_(false) {
  init_workers();
}

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

template <class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args)
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
  double totalRate = 0;
  for (size_t i = 1; i < copy.size(); ++i) {
    double distanceChange =
        copy[i].distance_to_ref_ - copy[i - 1].distance_to_ref_;
    int yearChange = copy[i].year_ - copy[i - 1].year_;

    if (yearChange == 0) {
      // Prevent division by zero
      continue;
    }

    totalRate += distanceChange / yearChange;
  }
  return totalRate / (double)(copy.size() - 1);
}
void save_points(const std::vector<gm::IntersectPoint> &shapes,
                 const std::filesystem::path &output_path) {
  // Step 1: Initialize GDAL
  GDALAllRegister();

  // Step 2: Get the shapefile driver
  GDALDriver *driver =
      GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
  if (driver == nullptr) {
    throw std::runtime_error("Unable to get ESRI Shapefile driver");
  }

  // Step 3: Create a new shapefile
  GDALDataset *dataset =
      driver->Create(output_path.string().c_str(), 0, 0, 0, GDT_Unknown, NULL);

  // Step 4: Create a layer for the shapefile
  OGRLayer *layer = dataset->CreateLayer("pointLayer", NULL, wkbPoint, NULL);

  // define attributes
  OGRFieldDefn baseline_id("Baseline_id", OFTInteger);
  if (layer->CreateField(&baseline_id) != OGRERR_NONE) {
    std::cerr << "Failed to create Name field" << std::endl;
    exit(1);
  }
  OGRFieldDefn transect_id("Transect_id", OFTInteger);
  if (layer->CreateField(&transect_id) != OGRERR_NONE) {
    std::cerr << "Failed to create Name field" << std::endl;
    exit(1);
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
    feature->SetField("Baseline_id", shape.baseline_id_);
    feature->SetField("Transect_id", shape.transect_id_);
    OGRFeature::DestroyFeature(feature);
  }


  // Clean up
  GDALClose(dataset);
}

}  // namespace util
