//
// Created by lby on 10/12/23.
//
#include "DSAS_cpp.h"
#include "geometry.h"

#include <iostream>
#include <algorithm>

namespace dsas {


    void digital_shoreline_analysis_system(const Path &folder, const Path &output_path) {
        // check the input
        std::vector<Path> paths;
        try {
            if (std::filesystem::exists(folder) && std::filesystem::is_directory(folder)) {
                for (const auto &entry: std::filesystem::directory_iterator(folder)) {
                    if (std::filesystem::is_regular_file(entry.path())) {
                        paths.push_back(entry.path());
                        std::cout << entry.path() << "\n";
                    }
                }
            } else {
                std::cerr << "Folder is not exist!\n";
            }
        } catch (std::filesystem::filesystem_error &err) {
            std::cerr << "Error: " << err.what() << "\n";
        }

        // start to analysis
        controller(paths, output_path);
    }


    void digital_shoreline_analysis_system(const std::vector<Path> &paths, const Path &output_path) {
        // check the input
        try {
            for (const auto &path: paths) {
                if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
                    std::cerr << "Path: " << path << "not exist or not a file!\n";
                }
            }
        } catch (std::filesystem::filesystem_error &err) {
            std::cerr << "Error: " << err.what() << "\n";
        }

        controller(paths, output_path);
    }

    void controller(const std::vector<Path> &paths, const Path &output_path) {
        // read the image
        std::vector<std::unique_ptr<Image>> images;
        for (const auto &path: paths) {
            images.emplace_back(std::make_unique<Image>(path));
        }

        // generate the baselines
        auto baselines = generate_baselines(images);

        // generate the transects
        auto transects = generate_transects(baselines);

        // genreate the intersections

    }

    Baselines generate_baselines(const std::vector<std::unique_ptr<Image>> &images) {
        // using the nearest the image as the baseline, since it is most eroded
        const auto img = std::max_element(images.begin(), images.end(), [](const auto &image1, const auto &image2) {
            return image1->year_ > image2->year_;
        });

        auto shorelines = img->get()->shorelines_;

        std::vector<gm::Baseline> baselines;
        int baseline_id{};
        for (const auto &shoreline: shorelines) {
            baselines.emplace_back(shoreline, 1000, 100, baseline_id++, 0);
        }
        return baselines;
    }

    TransectGroups generate_transects(const Baselines &baselines) {
        TransectGroups transectGroups{};
        for (auto &baseline: baselines) {
            Transects transects{
                baseline.baseline_id_,
                baseline.transects_lines_
            };
            transectGroups.push_back(transects);
        }
        return transectGroups;
    }
}
