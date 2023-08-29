//
// Created by lby on 8/9/23.
//
#include <iostream>
#include "util.h"

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
        for(auto &worker: workers_){
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

} // util
