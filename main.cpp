#include <iostream>
#include "geometry.h"
#include <opencv2/opencv.hpp>
#include "CImg.h"
#include "image.h"
#include <chrono>
#include <thread>

using namespace cv;
void func(std::string& path);

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::string path1 = "/home/lby/Desktop/shorecalculator/images/ExamplePNGs/5_5_m_4708733_ne_16_h_20160725.png";
    std::string path2 = "/home/lby/Desktop/shorecalculator/images/ExamplePNGs/5_6_m_4208226_ne_17_h_20160804.png";
    std::string path3 = "/home/lby/Desktop/shorecalculator/images/ExamplePNGs/5_6_m_4208227_ne_17_h_20160806.png";

    std::thread t1(func, std::ref(path1));
    std::thread t2(func, std::ref(path2));
    std::thread t3(func, std::ref(path3));
    t1.join();
    t2.join();
    t3.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Execution time: " << duration.count() << " milliseconds" << std::endl;
//    std::vector<double> x, y;
//
//    using namespace matplot;
//
//    for (auto &shore: *shorelines) {
//        for (auto &point: *shore.shore_ptr_) {
//            x.push_back(point->x);
//            y.push_back(point->y);
//        }
//        plot(x, y);
//        x.clear();
//        y.clear();
//    }
//
//    show();
//    delete shorelines;

    return 0;
}

void func(std::string& path){
    auto image = imread(path, 1);
    auto contour = im::extract_contours_water(path);
    auto *shorelines = new std::vector<gm::Shorelines>();
    im::extract_shorelines(contour, image.rows - 1, image.cols - 1, *shorelines);
}