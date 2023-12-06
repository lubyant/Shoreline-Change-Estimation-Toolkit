//
// Created by lby on 10/20/23.
//
#include "geometry.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "utility.h"

namespace gm {
int LineSegment::num_lines = 0;

LineSegment::LineSegment(Point<> leftEdge, Point<> rightEdge)
    : leftEdge_(leftEdge), rightEdge_(rightEdge) {
  num_lines++;
  double dx = rightEdge.x - leftEdge.x, dy = rightEdge.y - leftEdge.y;
  slope_ = dy / (dx + EPS_OFFSET);
  slope_vector_.first = dy;
  slope_vector_.second = dx;

  normal_vector_.first = slope_vector_.second;
  normal_vector_.second = -slope_vector_.first;
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
    leftEdge_.move_point(normal_vector_, dest);
    rightEdge_.move_point(normal_vector_, dest);
  }
}

bool LineSegment::is_intersect(const Point<> &point1,
                               const Point<> &point2) const {
  return util::isTwoSegmentIntersected<>(leftEdge_, rightEdge_, point1, point2);
}

Point<> LineSegment::find_intersection(const Point<> &point1,
                                       const Point<> &point2) const {
  if (!is_intersect(point1, point2)) throw std::runtime_error("Not intersect");
  return util::computeIntersectPoint<double>(leftEdge_, rightEdge_, point1,
                                             point2);
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const Point<T> &point) {
  os << "x: " << point.x << ", y: " << point.y;
  return os;
}

/**
 *  static members
 */
// cumulative distance from the first object of baselineSeg to last the transect
double BaselineSeg::cumulative_transects_distance = 0;
// cumulative distance  of baselineSeg
double BaselineSeg::cumulative_segment_distance = 0;

BaselineSeg::BaselineSeg(double spacing, double offset, const Point<> &leftEdge,
                         const Point<> &rightEdge)
    : LineSegment(leftEdge, rightEdge), spacing_(spacing), offset_(offset) {
  move_line(offset_);
  double length = leftEdge.distance_to_point(rightEdge);

  // if current baselineSeg is too short, don't create any transects
  if (cumulative_transects_distance + spacing_ >
      cumulative_segment_distance + length) {
    cumulative_segment_distance += length;
    return;
  }

  // starting length
  double start =
      cumulative_transects_distance + spacing_ - cumulative_segment_distance;
  double ratio = start / length;

  // left edge
  double x_l{leftEdge.x}, y_l{leftEdge.y};
  // right edge
  double x_r{rightEdge.x}, y_r{rightEdge.y};
  // start point
  double x_start{x_l + ratio * (x_r - x_l)};
  double y_start{y_l + ratio * (y_r - y_l)};
  gm::Point p0{x_start, y_start};

  // number of step
  int num = floor((p0.distance_to_point(rightEdge) + spacing_) / spacing_);

  // step size
  double dist = sqrt(slope_vector_.second * slope_vector_.second +
                     slope_vector_.first * slope_vector_.first);
  double x_step = spacing * slope_vector_.second / dist;
  double y_step = spacing * slope_vector_.first / dist;
  double x_cur, y_cur;
  for (int i = 0; i < num; i++) {
    x_cur = x_start + i * x_step;
    y_cur = y_start + i * y_step;
    transects_base_points_.emplace_back(x_cur, y_cur);
    // update the cumulative length
    cumulative_transects_distance += spacing_;
  }
  cumulative_segment_distance += length;
}

LineSegment TransectLine::create_transect(
    Point<> &transect_base, std::pair<double, double> baseline_normal_vector,
    double transect_length) {
  auto leftEdge =
      transect_base.create_point(baseline_normal_vector, transect_length / 2);
  auto rightEdge =
      transect_base.create_point(baseline_normal_vector, -transect_length / 2);

  return {leftEdge, rightEdge};
}

std::optional<IntersectPoint> TransectLine::intersection(
    const Shoreline &shoreline) const {
  std::vector<IntersectPoint> intersections;

  // find out all the available intersection
  for (size_t i = 0; i < shoreline.size() - 1; i++) {
    if (is_intersect(shoreline[i], shoreline[i + 1])) {
      auto point = find_intersection(shoreline[i], shoreline[i + 1]);
      auto distance = distance2ref(point);
      IntersectPoint intersect_point{
          point,        transect_id_,    shoreline.shoreline_id_,
          baseline_id_, shoreline.year_, distance};
      intersections.push_back(intersect_point);
    }
  }

  // if no intersection
  if (intersections.empty()) {
    return std::nullopt;
  }

  // if only one intersection
  if (intersections.size() == 1) {
    return intersections[0];
  }

  // if more than two intersections, pick one base on the intersection mode
  std::sort(intersections.begin(), intersections.end(),
            [](const IntersectPoint &a, const IntersectPoint &b) {
              return a.distance_to_ref_ < b.distance_to_ref_;
            });

  if (mode_ ==
      IntersectionMode::Farthest) {  // farthest mode return farthest distance
    return intersections[intersections.size() - 1];
  } else {  // close mode return smallest dis
    return intersections[0];
  }
}

Baseline::Baseline(const std::vector<BaselinesVertex> &points,
                   double transect_length, double spacing, int baseline_id,
                   double offset, int smooth_factor, gm::IntersectionMode mode)
    : baseline_id_(baseline_id),
      spacing_(spacing),
      transect_length_(transect_length),
      offset_(offset) {
  // create the baselineSeq
  int transect_id{0};
  if(smooth_factor < 1){
    throw std::runtime_error("smooth factor should no less than 1");
  }
  for (size_t i = 0; i < points.size() - smooth_factor; i+= smooth_factor) {
    BaselineSeg baselineSeg{spacing_, offset_, points.at(i),
                            points.at(i + smooth_factor)};
    if (i == 0) {
      transects_base_points_.push_back(baselineSeg.leftEdge_);
      transects_lines_.emplace_back(baselineSeg.leftEdge_, transect_length_,
                                    baselineSeg.normal_vector_, transect_id++,
                                    baseline_id_, mode);
      baseline_vertices_.push_back(baselineSeg.leftEdge_);
    }
    baseline_vertices_.push_back(baselineSeg.rightEdge_);
    for (auto &point : baselineSeg.transects_base_points_) {
      transects_base_points_.push_back(point);
      transects_lines_.emplace_back(point, transect_length_,
                                    baselineSeg.normal_vector_, transect_id++,
                                    baseline_id_);
    }
  }
}

}  // namespace gm