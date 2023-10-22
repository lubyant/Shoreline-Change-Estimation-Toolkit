#include <iostream>
#include "DSAS_cpp.h"
#include "image.h"
int main(int argc, char **argv) {
//    using namespace dsas;
//    // no input
//    if (argc == 0) {
//        std::cerr << "Please input the target path!\n";
//    }
//
//    // input folder
//    if (argc == 1) {
//        Path input_folder{argv[0]};
//        Path output_folder{argv[1]};
//        digital_shoreline_analysis_system(input_folder, output_folder);
//    }
//
//    // input paths
//    if (argc > 1) {
//        std::vector<Path> input_paths(argc-1);
//        for (int i = 0; i < argc - 1; i++) {
//            input_paths.emplace_back(argv[i]);
//        }
//        Path output_path{argv[argc-1]};
//        digital_shoreline_analysis_system(input_paths, output_path);
//    }

    using namespace std;
    filesystem::path image_path = "/home/lby/Desktop/DSAS_cpp/img/final_raster/4108603/4108603_2018.tif";
//    filesystem::path image_path{"/home/lby/Desktop/DSAS_cpp/img/ExamplePNGs/5_5_m_4708733_ne_16_h_20160725.png"};
    filesystem::path output_path{"baseline1.shp"};
    auto *img = new dsas::Image(image_path);
    std::vector<std::vector<gm::Point<int>>> lines;
    for(auto& contour: img->shorelines_){
        std::vector<gm::Point<int>> line{};
        for(auto& point: contour){
            line.emplace_back(point.x, point.y);
        }
//        line.emplace_back(contour[0].x, contour[0].y);
        lines.push_back(line);
    }
//    util::save_lines<std::vector<gm::Point<int>>>(lines, output_path);

    std::vector<std::unique_ptr<dsas::Image>> images;
    images.push_back(make_unique<dsas::Image>(*img));
    auto baselines = dsas::generate_baseline(images);

    util::save_lines<gm::Baseline>(baselines, output_path);

    delete img;
    return 0;
}
