//
// Created by lby on 8/9/23.
//

#ifndef SHORECALCULATOR_UTIL_H
#define SHORECALCULATOR_UTIL_H

#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <future>

namespace util {

    class ThreadPool {
    public:
        ThreadPool();

        explicit ThreadPool(uint32_t num_threads);

        ~ThreadPool();

        template<class F, class... Args>
        auto enqueue(F &&f, Args &&... args)
            -> std::future<typename std::invoke_result<F, Args...>::type>;

    private:
        uint32_t num_threads_;
        bool stop_;

        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;

        void init_workers();
    };

} // util

#endif //SHORECALCULATOR_UTIL_H
