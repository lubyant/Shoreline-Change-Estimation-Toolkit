#include <iostream>
#include "geometry.h"
#include <opencv2/opencv.hpp>
#include "CImg.h"
#include "image.h"
#include <chrono>
#include <thread>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

//int main()
//{
//    // Step 1: Initialize GDAL
//    GDALAllRegister();
//
//    // Step 2: Get the shapefile driver
//    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
//
//    // Step 3: Create a new shapefile
//    GDALDataset *dataset = driver->Create("line.shp", 0, 0, 0, GDT_Unknown, NULL);
//
//    // Step 4: Create a layer for the shapefile
//    OGRLayer *layer = dataset->CreateLayer("line", NULL, wkbLineString, NULL);
//
//    // Step 5: Create a new feature
//    OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
//
//    // Step 6: Create a line geometry and add points to it
//    OGRLineString line;
//    line.addPoint(0.0, 0.0);
//    line.addPoint(1.0, 1.0);
//
//    // Step 7: Add the geometry to the feature
//    feature->SetGeometry(&line);
//
//    // Step 8: Add the feature to the layer
//    layer->CreateFeature(feature);
//
//    // Clean up
//    OGRFeature::DestroyFeature(feature);
//    GDALClose(dataset);
//
//    return 0;
//}

using namespace cv;
using namespace std;

void func(std::string &path);

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

void func(vector<string> &paths) {
    vector<vector<gm::Shorelines>> shores_years;
    const double transect_length = 1000.00;
    const double spacing = 10.00;
    const double offset = 0.00;
    for (const auto &path: paths) {
        auto image = imread(path, 1);
        auto contour = im::extract_contours_water(path);
        auto shores = im::extract_shorelines(contour, image.rows - 1, image.cols - 1, 2000);
        shores_years.push_back(shores);
    }
    auto&& baseline = im::create_baseline(shores_years[0], transect_length, spacing, 0);
    auto intersections = im::create_intersections(baseline, shores_years[1]);

}