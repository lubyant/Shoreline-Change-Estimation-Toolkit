#include <iostream>
#include "geometry.h"
#include <cmath>
#include <opencv2/opencv.hpp>

#include <matplot/matplot.h>
#include "CImg.h"
//int main() {
//    using namespace gm;
//    Point point1(0, 0), point2(1, 0), point3(1, 1), point4(0, 2);
//    auto *points = new std::vector<Point>();
//    points->push_back(point1);
//    points->push_back(point2);
//    points->push_back(point3);
//    points->push_back(point4);
//
//    Baselines baselines(*points, 10, 0.3, 0, 0);
//    std::vector<double> x, y;
//    for (auto & i : *baselines.transects_){
//        x.push_back(i.x);
//        y.push_back(i.y);
//    }
//    matplot::scatter(x, y);
//    matplot::show();
//    return 0;
//}
//int main() {
//    using namespace matplot;
//    std::vector<double> x = linspace(0, 2 * pi);
//    std::vector<double> y = transform(x, [](auto x) { return sin(x); });
//
//    plot(x, y, "-o");
//    hold(on);
//    plot(x, transform(y, [](auto y) { return -y; }), "--xr");
//    plot(x, transform(x, [](auto x) { return x / pi - 1.; }), "-:gs");
//    plot({1.0, 0.7, 0.4, 0.0, -0.4, -0.7, -1}, "k");
//
//    show();
//    return 0;
//}

//int main(){
//    char path[] = "/home/lby/Desktop/shorecalculator/images/ExamplePNGs/5_5_m_4708733_ne_16_h_20160725.png";
//    auto image = matplot::imread(path);
//
//    matplot::imshow(image);
//    matplot::show();
//    return 0;
//}

//#include <ogrsf_frmts.h>
//
//int main()
//{
//    GDALAllRegister();
//
//    const char *pszDriverName = "ESRI Shapefile";
//    GDALDriver *poDriver;
//
//    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
//    if (poDriver == NULL)
//    {
//        std::cout << pszDriverName << " driver not available." << std::endl;
//        exit(1);
//    }
//
//    GDALDataset *poDS;
//    poDS = poDriver->Create("line.shp", 0, 0, 0, GDT_Unknown, NULL);
//    if (poDS == NULL)
//    {
//        std::cout << "Creation of output file failed." << std::endl;
//        exit(1);
//    }
//
//    OGRLayer *poLayer;
//    poLayer = poDS->CreateLayer("line", NULL, wkbLineString, NULL);
//    if (poLayer == NULL)
//    {
//        std::cout << "Layer creation failed." << std::endl;
//        exit(1);
//    }
//
//    OGRFeature *poFeature;
//    poFeature = OGRFeature::CreateFeature(poLayer->GetLayerDefn());
//    if (poFeature == NULL)
//    {
//        std::cout << "Feature creation failed." << std::endl;
//        exit(1);
//    }
//
//    OGRLineString oLine;
//    oLine.addPoint(1.0, 1.0);
//    oLine.addPoint(2.0, 2.0);
//    oLine.addPoint(3.0, 3.0);
//
//    poFeature->SetGeometry(&oLine);
//
//    if (poLayer->CreateFeature(poFeature) != OGRERR_NONE)
//    {
//        std::cout << "Failed to create feature in shapefile." << std::endl;
//        exit(1);
//    }
//
//    OGRFeature::DestroyFeature(poFeature);
//    GDALClose(poDS);
//
//    return 0;
//}
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "image.h"
#include <chrono>

using namespace cv;

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    Mat image;
    std::string path = "/home/lby/Desktop/shorecalculator/images/ExamplePNGs/5_5_m_4708733_ne_16_h_20160725.png";
    image = imread(path, 1);
    if (!image.data) {
        printf("No image data \n");
        return -1;
    }

    auto contour = im::extract_contours_water(path);
    std::vector<gm::Shorelines>shorelines{};
    im::extract_shorelines(contour, image.rows - 1, image.cols - 1, shorelines);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Execution time: " << duration.count() << " milliseconds" << std::endl;
    std::vector<double> x, y;

    using namespace matplot;

    for (auto &shore: shorelines) {
        for (auto &point: *shore.shore_ptr_) {
            x.push_back(point->x);
            y.push_back(point->y);
        }
        plot(x, y);
        x.clear();
        y.clear();
    }

    show();

    return 0;
}