//
// Created by lby on 10/20/23.
//
#include "geometry.h"
#include <iostream>
#include <cmath>

#include "utility.h"
namespace gm {
    int LineSegment::num_lines = 0;
    LineSegment::LineSegment(Point<> leftEdge, Point<> rightEdge) :
            leftEdge_(leftEdge), rightEdge_(rightEdge) {
        num_lines++;
        double dx = rightEdge.x - leftEdge.x, dy = rightEdge.y - leftEdge.y;
        slope_ = dy / (dx + EPS_OFFSET);
        intercept_ = this->leftEdge_.y - slope_ * this->leftEdge_.x;
        if (dy >= 0) {
            if (dx >= 0) {
                orient_ = atan(fabs(slope_));
            } else {
                orient_ = atan(fabs(slope_)) + PI / 2;
            }
        } else {
            if (dx >= 0) {
                orient_ = 2 * PI - atan(fabs(slope_));
            } else {
                orient_ = PI + atan(fabs(slope_));
            }
        }
    }

    void LineSegment::move_line(double dest) {
        if (dest != 0) {
            double normalDir = orient_;
            leftEdge_.move_point(normalDir, dest);
            rightEdge_.move_point(normalDir, dest);
        }
    }

    bool LineSegment::is_intersect(Point<> &point1, Point<> &point2) const {

        return util::isTwoSegmentIntersected<>(leftEdge_, rightEdge_, point1, point2);

    }

    Point<> LineSegment::find_intersection(Point<> &point1, Point<> &point2) const {
        if (is_intersect(point1, point2))
            throw std::runtime_error("Not intersect");
        return util::computeIntersectPoint<double>(leftEdge_, rightEdge_, point1, point2);
    }


    template<typename T>
    std::ostream &operator<<(std::ostream &os, const Point<T> &point) {
        os << "x: " << point.x << ", y: " << point.y;
        return os;
    }


    BaselineSeg::BaselineSeg(double spacing, double offset,
                             const Point<> &leftEdge, const Point<> &rightEdge,
                             double spacing_leftover)
            : LineSegment(leftEdge, rightEdge), spacing_(spacing), offset_(offset),
              spacing_leftover_(spacing_leftover) {
        move_line(offset_);
        double length = leftEdge.distance_to_point(rightEdge);
        double start = spacing_leftover_;
        double ratio = start / length;


        if (spacing_ < length - start) {
            double x_l{leftEdge.x}, y_l{leftEdge.y};
            double x_r{rightEdge.x}, y_r{rightEdge.y};
            double x_start{x_l + ratio * (x_r - x_l)};
            double y_start{y_l + ratio * (y_r - y_l)};
            gm::Point p0{x_start, y_start};
            int num = floor((p0.distance_to_point(rightEdge) + spacing) / spacing_);
            double x_step = sqrt(spacing_ * spacing_ / (1 + slope_ * slope_)) *
                            fabs(x_r - x_l) / (x_r - x_l + EPS_OFFSET);
            double y_step =
                    sqrt(slope_ * slope_ * spacing_ * spacing_ / (1 + slope_ * slope_));
            double x_cur, y_cur;
            for (int i = 0; i < num; i++) {
                x_cur = x_start + i * x_step;
                y_cur = y_start + i * y_step;
                transects_base_points_.emplace_back(x_cur, y_cur);
            }
            spacing_leftover_ =
                    spacing_ - sqrt((rightEdge.x - x_cur) * (rightEdge.x - x_cur) +
                                    (rightEdge.y - y_cur) * (rightEdge.y - y_cur));
        }
    }


    LineSegment TransectLine::create_transect(Point<> &transect_base, double baseline_orient, double transect_length) {
        gm::Point leftEdge = std::move(
                transect_base.create_point(baseline_orient - PI / 2, transect_length / 2));
        gm::Point rightEdge = std::move(
                transect_base.create_point(baseline_orient + PI / 2, transect_length / 2));

        return {leftEdge, rightEdge};
    }

    Baseline::Baseline(const std::vector<BaselinesVertex> &points, double transect_length, double spacing,
                       int baseline_id, double offset, int smooth_factor) :
            baseline_id_(baseline_id), spacing_(spacing), transect_length_(transect_length), offset_(offset) {
       // create the baselineSeq
       double spacing_leftover{0};
       size_t num_lines{points.size()-1};
       int transect_id{0};

       for(size_t i=0; i<num_lines; i += smooth_factor){
           BaselineSeg baselineSeg{spacing_, offset_, points[i], points[i+smooth_factor], spacing_leftover};
           spacing_leftover = baselineSeg.spacing_leftover_;
           for(auto& point : baselineSeg.transects_base_points_){
              transects_base_pints_.push_back(point);
              transects_lines_.emplace_back(point, transect_length, baselineSeg.orient_, transect_id);
           }
       }

    }

}