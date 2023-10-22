//
// Created by lby on 10/13/23.
//
#include <iostream>
#include <functional>
#include "utility.h"

namespace util {

    ThreadPool::ThreadPool() : num_threads_(std::thread::hardware_concurrency()), stop_(false) {
        init_workers();
    }

    ThreadPool::ThreadPool(uint32_t num_threads) : num_threads_(num_threads), stop_(false) {
        init_workers();
    }

    void ThreadPool::init_workers() {
        for (uint32_t i = 0; i < num_threads_; i++) {
            workers_.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex_);
                        this->condition_.wait(lock, [this]() { return this->stop_ || !this->tasks_.empty(); });
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
        for (auto &worker: workers_) {
            worker.join();
        }
    }

    template<class F, class... Args>
    auto ThreadPool::enqueue(F &&f, Args &&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_t = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_t()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
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

    double linearRegressRate(const std::vector<double> &years,
                             const std::vector<double> &distances) {
        u_long n = years.size();
        double mean_year =
                std::accumulate(years.begin(), years.end(), 0.0) / (double) years.size();
        double mean_dis =
                std::accumulate(years.begin(), years.end(), 0.0) / (double) years.size();

        // Calculating cross-deviation and deviation of x
        double num = 0.0, den = 0.0;
        for (int i = 0; i < n; i++) {
            num += (years[i] - mean_year) * (distances[i] - mean_dis);
            den += (years[i] - mean_year) * (years[i] - mean_year);
        }
        return num / den;
    }




    void save_points(std::vector<gm::Point<double>> &shapes, std::filesystem::path &output_path) {
        // Step 1: Initialize GDAL
        GDALAllRegister();

        // Step 2: Get the shapefile driver
        GDALDriver *driver =
                GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");

        // Step 3: Create a new shapefile
        GDALDataset *dataset =
                driver->Create(output_path.string().c_str(), 0, 0, 0, GDT_Unknown, NULL);

        // Step 4: Create a layer for the shapefile
        OGRLayer *layer = dataset->CreateLayer("pointLayer", NULL, wkbPoint, NULL);

        // Step 5: Create a new feature
        OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());

        // Step 6: Create a line geometry and add points to it
        OGRPoint point;
        for (const auto &shape: shapes) {
            point.setX(shape.x);
            point.setY(shape.y);
        }

        // Step 7: Add the geometry to the feature
        feature->SetGeometry(&point);
        OGRFeature::DestroyFeature(feature);

        // Clean up
        GDALClose(dataset);
    }
} // util
