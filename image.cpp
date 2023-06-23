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

    std::vector<std::vector<cv::Point>> extract_contours_water(std::string &path){
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

    std::vector<gm::Shorelines> extract_shorelines(std::vector<std::vector<cv::Point>> &contours, int x_lim, int y_lim){
        std::vector<gm::Shorelines> shores_inventory;
        for(auto& contour: contours){
            unsigned long num = contours.size();
            auto shore = gm::Shorelines();
            bool isEdgeCut = false;
            std::vector<gm::Shorelines> temp;
            for (unsigned long i=0; i<num; i++){
                auto cur_x = contour[i].x, cur_y = contour[i].y;

                if (!IsEdge(cur_x, cur_y, x_lim, y_lim)){
                    gm::Point left = shore.lines_vec_ptr_->at(shore.size()-1).rightEdge;
                    gm::Point right = gm::Point(cur_x, cur_y);
                    shore.pushBack(gm::LineSegment(left, right));
                } else {
                    isEdgeCut = true;
                    if (shore.size() > 0){
                        temp.push_back(shore);
                        shore.lines_vec_ptr_->clear();
                    }
                    continue;
                }
            }
            auto start_x = contour[0].x, start_y = contour[0].y;
            auto end_x = contour[contour.size()-1].x, end_y = contour[contour.size()-1].y;
            if (!isEdgeCut){
                continue;
            } else {
                if (shore.size()>0){
                    temp.push_back(shore);
                }
            }

            if (isEdgeCut && temp.size() > 1 && !(IsEdge(start_x, start_y, x_lim, y_lim) || IsEdge(end_x, end_y, x_lim, y_lim))){
                auto front_shore = temp[0];
                auto end_shore = temp[temp.size()-1];
                temp.pop_back();

                auto end_shore_num = end_shore.size();
                for (unsigned long i = 0; i < end_shore_num; i++){
                    gm::Point left = end_shore.lines_vec_ptr_->at(end_shore_num-1-i).leftEdge;
                    gm::Point right = front_shore.lines_vec_ptr_->at(0).rightEdge;
                    front_shore.pushFront(gm::LineSegment(left, right));
                }
                temp.erase(temp.begin());
                temp.insert(temp.begin(), front_shore);
            }
            if(!temp.empty()){
                shores_inventory.insert(shores_inventory.end(), temp.begin(), temp.end());
                temp.clear();
            }
        }
        return shores_inventory;
    }


} // im