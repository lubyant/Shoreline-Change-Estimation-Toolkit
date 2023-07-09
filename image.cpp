//
// Created by lby on 6/18/23.
//

#include "image.h"

#define IsEdge(x_cor, y_cor, x_lim, y_lim) ((x_cor) == 0 || (x_cor) == x_lim || (y_cor) == 0 || (y_cor) == y_lim)


namespace im {
    std::vector<std::string> read_files(std::string &path) {
        std::vector<std::string> path_string;
        std::string search_path = path + "/*.*";
        for (const auto &entry: std::filesystem::recursive_directory_iterator(path)) {
            if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg" ||
                entry.path().extension() == ".jpeg" || entry.path().extension() == ".tiff") {
                path_string.push_back(entry.path().string());
            }
        }
        return path_string;
    }

    std::vector<std::vector<cv::Point>> extract_contours_water(std::string &path) {
        // read the image
        cv::Mat img = cv::imread(path);

        // grey scale
        cv::Mat gray;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

        // threshold
        cv::Mat thresh;
        cv::threshold(gray, thresh, 1, 255, cv::THRESH_BINARY);

        // contour
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(thresh, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

        return contours;
    }

    void
    extract_shorelines(std::vector<std::vector<cv::Point>> &contours, int x_lim, int y_lim,
                       std::vector<gm::Shorelines> &shores_inventory, int year) {
        bool isEdgeCut = false;
        for (auto &contour: contours) {
            unsigned long num = contour.size();
//            auto shore = std::make_unique<gm::Shorelines>();
            gm::Shorelines shore{};
            shore.year = year;
//            auto temp = std::make_unique<std::vector<gm::Shorelines>>();
            std::vector<gm::Shorelines> temp{};
            for (unsigned long i = 0; i < num; i++) {
                auto cur_x = contour[i].x, cur_y = contour[i].y;

                if (!IsEdge(cur_x, cur_y, x_lim, y_lim)) {
                    shore.pushBack(cur_x, cur_y);
                } else {
                    isEdgeCut = true;
                    if (shore.size() > 0) {
                        temp.push_back(std::move(shore));
                        shore = gm::Shorelines();
                    }
                    continue;
                }
            }

            auto start_x = contour[0].x, start_y = contour[0].y;
            auto end_x = contour[contour.size() - 1].x, end_y = contour[contour.size() - 1].y;
            if (!isEdgeCut) {
                continue;
            } else {
                if (shore.size() > 0) {
                    temp.push_back(std::move(shore));
                }
            }

            if (isEdgeCut == true && temp.size() > 1 &&
                !(IsEdge(start_x, start_y, x_lim, y_lim) || IsEdge(end_x, end_y, x_lim, y_lim))) {

                auto front_shore = temp[0];
                auto end_shore = temp[temp.size() - 1];
                temp.pop_back();

                auto end_shore_num = end_shore.size();
                for (unsigned long i = 0; i < end_shore_num; i++) {
                    front_shore.pushFront(end_shore.shore_ptr_->at(end_shore_num - i - 1)->x,
                                          end_shore.shore_ptr_->at(end_shore_num - i - 1)->y);
                }

                temp.erase(temp.begin());

                temp.insert(temp.begin(), front_shore);

            }

            if (!temp.empty()) {
                shores_inventory.insert(shores_inventory.end(), temp.begin(), temp.end());
            }

        }
    }

    gm::Baselines
    create_baseline(std::vector<gm::Shorelines> &shores_inventory, double transects_length, double spacing,
                    int baseline_id, double offset) {
        using namespace gm;
        using namespace std;
        // sort
        sort(shores_inventory.begin(), shores_inventory.end(),
             [](const auto &a, const auto &b) { return a.year < b.year; });

        // use the earliest year as baseline
        vector<Point> baseline_points;
        for (const auto &p: *shores_inventory[0].shore_ptr_) {
            baseline_points.push_back(*std::make_unique<Point>(*p));
        }
        return {baseline_points, transects_length, spacing, baseline_id, offset};
    }

    void create_intersections(gm::Baselines &baselines, std::vector<gm::Shorelines> &shorelines) {
        for (auto &shoreline: shorelines) {
            baselines.intersect_shorelines(shoreline);
        }
    }


} // im