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
    filesystem::path image_path = "/home/lby/Desktop/shorecalculator/img/final_raster/4108603/4108603_2018.tif";
    filesystem::path output_path{"shoreline.shp"};
    auto *img = new dsas::Image(image_path);
    std::vector<std::vector<gm::Point<int>>> lines;
    for(auto& contour: img->shorelines_){
        std::vector<gm::Point<int>> line{};
        for(auto& point: contour){
            line.emplace_back(point.x, point.y);
        }
        lines.push_back(line);
    }
    util::save_lines<std::vector<gm::Point<int>>>(lines, output_path);

    std::vector<std::unique_ptr<dsas::Image>> images;
    images.push_back(make_unique<dsas::Image>(*img));
    auto baselines = dsas::generate_baselines(images);

    filesystem::path output_path_1{"baseline.shp"};
    util::save_lines<gm::Baseline>(baselines, output_path);

    filesystem::path output_path_2{"transects1.shp"};
    auto transect_groups = dsas::generate_transects(baselines);
    for(auto &transects : transect_groups){
        util::save_lines<gm::TransectLine>(transects.transects_, output_path_2);
    }

    delete img;
    return 0;
}
