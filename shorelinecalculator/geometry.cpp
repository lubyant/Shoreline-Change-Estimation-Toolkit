//
// Created by lby on 10/20/23.
//
#include "geometry.hpp"

#include <unordered_set>

#include "utility.hpp"

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

void TransectLine::truncate_shoreline_seg(const Shoreline &shoreline) {
  assert(prev_transect_line != nullptr && next_transect_line != nullptr);
  TransectLine *prev=prev_transect_line, *next=next_transect_line;
  if(prev == nullptr) {
    prev = this;
  }
  if(next == nullptr) {
    next = this;
  }
  auto sub_shore = util::trancate_shore_by_transect(*prev, *next, shoreline);
}
LineSegment TransectLine::create_transect(
    Point<> &transect_base, std::pair<double, double> baseline_normal_vector,
    double transect_length, TransectOrientation orient) {
  auto leftEdge{transect_base}, rightEdge{transect_base};
  switch (orient) {
    case TransectOrientation::Mix:
      leftEdge = transect_base.create_point(baseline_normal_vector,
                                            transect_length / 2);
      rightEdge = transect_base.create_point(baseline_normal_vector,
                                             -transect_length / 2);
      if (leftEdge == rightEdge) {
        std::cerr << baseline_normal_vector.first
                  << baseline_normal_vector.second << std::endl;
        std::cerr << leftEdge << ", " << rightEdge << std::endl;
        std::cerr << __FILE__ << std::endl;
        exit(1);
      }
      break;
    case TransectOrientation::Left:
      leftEdge =
          transect_base.create_point(baseline_normal_vector, transect_length);
      rightEdge = transect_base;
      break;
    case TransectOrientation::Right:
      leftEdge = transect_base;
      rightEdge =
          transect_base.create_point(baseline_normal_vector, -transect_length);
      break;
    default:
      throw std::runtime_error("Not a valid orientation!");
  }
  return {leftEdge, rightEdge};
}

std::optional<IntersectPoint> TransectLine::intersection(
    const Shoreline &shoreline) const {
  std::vector<IntersectPoint> intersections;

  // find out all the available intersection
  for (size_t i = 0; i < shoreline.size() - 1; i++) {
    if (is_intersect(shoreline[i], shoreline[i + 1])) {
      try {
        auto point = find_intersection(shoreline[i], shoreline[i + 1]);
        auto distance = distance2ref(point);
        IntersectPoint intersect_point{point,
                                       transect_id_,
                                       shoreline.shoreline_id_,
                                       baseline_id_,
                                       image_id_,
                                       shoreline.year_,
                                       distance,
                                       this,
                                       &shoreline[i],
                                       &shoreline[i + 1]};
        intersections.push_back(intersect_point);
      } catch (...) {
        continue;
      }
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

  if (mode_ == IntersectionMode::Farthest) {
    // farthest mode return farthest distance
    return intersections[intersections.size() - 1];
  } else {
    // close mode return smallest dis
    return intersections[0];
  }
}

void TransectLine::set_info(const std::vector<double> &years,
                            const std::vector<double> &distances) {
  if (years.size() != distances.size()) {
    throw std::runtime_error("size is not the same!");
  }

  num_intersect_ = years.size();
  std::stringstream ss;
  for (size_t i = 0; i < num_intersect_; i++) {
    ss << years[i] << ", " << distances[i] << ". ";
    intersect_info_ = ss.str();
    ss.clear();
  }

  // calculate the rate
  change_rate = util::least_square(years, distances);
}

Baseline::Baseline(const std::vector<BaselinesVertex> &points,
                   double transect_length, double spacing, int baseline_id,
                   int image_id, double offset, int smooth_factor,
                   gm::IntersectionMode mode, gm::TransectOrientation orient)
    : baseline_id_(baseline_id),
      image_id_(image_id),
      spacing_(spacing),
      transect_length_(transect_length),
      offset_(offset) {
  // create the baselineSeq
  int transect_id{0};
  std::vector<std::pair<double, double>> normal_vectors;
  for (size_t i = 0; i < points.size() - 1; i++) {
    BaselineSeg baselineSeg{spacing_, offset_, points.at(i), points.at(i + 1)};
    // starting point
    if (i == 0) {
      transects_base_points_.push_back(baselineSeg.leftEdge_);
      normal_vectors.push_back(baselineSeg.normal_vector_);
      baseline_vertices_.push_back(baselineSeg.leftEdge_);
    }
    baseline_vertices_.push_back(baselineSeg.rightEdge_);
    for (auto &point : baselineSeg.transects_base_points_) {
      transects_base_points_.push_back(point);
      normal_vectors.push_back(baselineSeg.normal_vector_);
    }
  }

  // smoothing the transects
  if (smooth_factor < 0) {
    throw std::runtime_error("smooth factor should no less than 0");
  }
  for (size_t i = 0; i < normal_vectors.size(); i++) {
    size_t start = i;
    size_t num = i + smooth_factor < normal_vectors.size()
                     ? smooth_factor
                     : normal_vectors.size() - start;

    double x = 0, y = 0;
    for (size_t i = start; i < start + num; i++) {
      x += normal_vectors.at(i).first;
      y += normal_vectors.at(i).second;
    }
    auto smoothed_normal_vector = std::make_pair(x, y);
    if (smoothed_normal_vector.first == 0 &&
        smoothed_normal_vector.second == 0) {
      smoothed_normal_vector.first = normal_vectors.at(i).first;
      smoothed_normal_vector.second = normal_vectors.at(i).second;
    }
    transects_lines_.emplace_back(
        transects_base_points_.at(i), transect_length_, smoothed_normal_vector,
        transect_id++, baseline_id_, image_id_, mode, orient);
  }

  for (size_t i = 0; i < transects_lines_.size(); i++) {
    if (i == 0) {
      transects_lines_[i].next_transect_line = &transects_lines_[i + 1];
    } else if (i == transects_lines_.size() - 1) {
      transects_lines_[i].prev_transect_line = &transects_lines_[i - 1];
    } else {
      transects_lines_[i].next_transect_line = &transects_lines_[i + 1];
      transects_lines_[i].prev_transect_line = &transects_lines_[i - 1];
    }
  }
}
Shoreline::Shoreline(std::vector<gm::Point<double>> &shoreline_vertices,
                     int shoreline_id, int year, int image_id)
    : shoreline_vertices_(shoreline_vertices),
      shoreline_id_(shoreline_id),
      year_(year),
      image_id_(image_id) {
  date_ = boost::gregorian::date(year_, 1, 1);
}
Shoreline::Shoreline(std::vector<gm::Point<double>> &shoreline_vertices,
                     int shoreline_id, boost::gregorian::date date,
                     int image_id)
    : shoreline_vertices_(shoreline_vertices),
      shoreline_id_(shoreline_id),
      date_(date),
      image_id_(image_id) {
  year_ = date_.year();
}

// ShoreSegByTransect::ShoresIterator::value_type
// ShoreSegByTransect::ShoresIterator::operator*() const {
//   auto transect_group = shore_seg_.transect_groups_.at(baseline_pos_);
//   size_t max_baseline_pos = transect_group.transects_.size() - 1;
//   auto transect_prev = transect_group.transects_.at(transect_pos_);
//   auto transect_next = transect_group.transects_.at(
//       std::min(transect_pos_ + 1, max_baseline_pos));
//   std::unordered_set<int> years_prev, years_next;
//   for (const auto &pair : transect_prev.year_intersect_map_) {
//     years_prev.insert(pair.first);
//   }
//   for (const auto &pair : transect_next.year_intersect_map_) {
//     years_next.insert(pair.first);
//   }
//   std::unordered_set<int> shared_years;
//   std::set_intersection(years_next.begin(), years_next.end(),
//                         years_prev.begin(), years_prev.end(),
//                         std::inserter(shared_years, shared_years.begin()));
//   value_type ret;
//   for (const auto &year : shared_years) {
//     auto intersect_prev = transect_prev.year_intersect_map_[year];
//     auto intersect_next = transect_next.year_intersect_map_[year];
//     for (const auto &shoreline : shore_seg_.shorelines_) {
//       if (shoreline.image_id_ != transect_prev.image_id_) {
//         continue;
//       }
//       ret[year] = util::get_subset_of_vertices(
//           shoreline.shoreline_vertices_, *intersect_prev, *intersect_next);
//     }
//   }
//   return ret;
// }
// ShoreSegByTransect::ShoresIterator &
// ShoreSegByTransect::ShoresIterator::operator++() {
//   if (transect_pos_ ==
//       shore_seg_.transect_groups_[baseline_pos_].transects_.size() - 1) {
//     if (baseline_pos_ != shore_seg_.transect_groups_.size() - 1) {
//       baseline_pos_++;
//       transect_pos_ = 0;
//     } else {
//       throw std::runtime_error("line: 333");
//     }
//   } else {
//     transect_pos_++;
//   }
//   return *this;
// }
}  // namespace gm