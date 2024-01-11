//
// Created by lby on 10/20/23.
//

#ifndef DSAS_CPP_GEOMETRY_H
#define DSAS_CPP_GEOMETRY_H

#define EPS_OFFSET 1e-6
#define PI 3.1415926

#include <ogr_core.h>

#include <cmath>
#include <iostream>
#include <optional>
#include <tuple>
#include <vector>

// classes
namespace gm {

template <typename T = double>
struct Point;

struct Shoreline;
struct IntersectPoint;

template <typename T>
std::ostream &operator<<(std::ostream &os, const Point<T> &point);

template <typename T>
struct MultiLine {
  [[nodiscard]] virtual size_t size() const = 0;

  [[nodiscard]] virtual const T &operator[](size_t i) const = 0;

  virtual ~MultiLine() = default;
};

enum class IntersectionMode { Closest, Farthest };

template <typename... Arg>
struct GDALShpSaver {
  [[nodiscard]] virtual std::vector<std::string> get_names() const = 0;
  [[nodiscard]] virtual std::vector<OGRFieldType> get_types() const = 0;
  [[nodiscard]] virtual std::tuple<Arg...> get_values() const = 0;
  virtual ~GDALShpSaver() = default;
};

template <typename T>
struct Point {
  T x, y;

  Point(T x, T y) : x(x), y(y) {}

  friend std::ostream &operator<< <T>(std::ostream &os, const Point<T> &point);

  [[nodiscard]] T distance_to_point(const Point<T> &point) const {
    return sqrt(pow(x - point.x, 2) + pow(y - point.y, 2));
  }

  void move_point(std::pair<double, double> orient, T dest) {
    double dist =
        sqrt(orient.first * orient.first + orient.second * orient.second);
    y += (T)(dest * orient.first / dist);
    x += (T)(dest * orient.second / dist);
  }

  Point<T> create_point(std::pair<double, double> orient, T dest) {
    double dist =
        sqrt(orient.first * orient.first + orient.second * orient.second);
    double y_new = y + dest * orient.first / dist;
    double x_new = x + dest * orient.second / dist;
    return {x_new, y_new};
  }
};

struct LineSegment {
  Point<> leftEdge_, rightEdge_;
  double slope_, orient_;
  static int num_lines;
  std::pair<double, double> slope_vector_, normal_vector_;  // {y, x}

  // constructor
  LineSegment(Point<> leftEdge, Point<> rightEdge);

  // destructor
  virtual ~LineSegment() { num_lines--; }

  // print
  friend std::ostream &operator<<(std::ostream &os,
                                  const LineSegment &lineSegment) {
    os << "left edge: " << lineSegment.leftEdge_
       << ", right edge: " << lineSegment.rightEdge_;
    return os;
  }

  // move the current line by distant in normal direction
  void move_line(double dest);

  [[nodiscard]] bool is_intersect(const Point<> &point1,
                                  const Point<> &point2) const;

  [[nodiscard]] Point<> find_intersection(const Point<> &point1,
                                          const Point<> &point2) const;
};

struct BaselineSeg : public LineSegment {
  using TransectBasePoint = Point<>;
  double spacing_, offset_;
  static double cumulative_transects_distance,
      cumulative_segment_distance;  // cumulative distance
  std::vector<TransectBasePoint> transects_base_points_;

  // constructor
  BaselineSeg(double spacing, double offset, const Point<> &leftEdge,
              const Point<> &rightEdge);

  // element accessing
  const TransectBasePoint &operator[](const unsigned int n) const {
    return transects_base_points_[n];
  }
};

#define transect_t int, int, int, double
struct TransectLine : public LineSegment,
                      MultiLine<Point<double>>,
                      GDALShpSaver<transect_t> {
  Point<double> transect_ref_point_;   // point to calculate the erosion
  Point<double> transect_base_point_;  // point to generate the shapefile
  int transect_id_;
  int baseline_id_;
  int image_id_;
  double change_rate{};  // change rate for all the intersections
  IntersectionMode mode_;

  TransectLine(Point<> &transect_base, double transect_length,
               std::pair<double, double> baseline_normal_vector,
               int transect_id, int baseline_id, int image_id,
               IntersectionMode mode = IntersectionMode::Closest)
      : transect_base_point_(transect_base),
        LineSegment(create_transect(transect_base, baseline_normal_vector,
                                    transect_length)),

        transect_ref_point_(rightEdge_),
        transect_id_(transect_id),
        baseline_id_(baseline_id),
        image_id_(image_id),
        mode_(mode) {}

  static LineSegment create_transect(
      Point<> &transect_base, std::pair<double, double> baseline_normal_vector,
      double transect_length);

  [[nodiscard]] std::optional<IntersectPoint> intersection(
      const Shoreline &shoreline) const;

  double distance2ref(Point<double> &point) const {
    return transect_ref_point_.distance_to_point(point);
  }

  [[nodiscard]] size_t size() const override { return 3; }

  [[nodiscard]] const Point<double> &operator[](size_t i) const override {
    switch (i) {
      case 0:
        return leftEdge_;
      case 1:
        return transect_ref_point_;
      case 2:
        return rightEdge_;
      default:
        throw std::runtime_error("Not a valid index");
    }
  }

  [[nodiscard]] std::vector<std::string> get_names() const override {
    return {"TransectId", "BaselineId", "ImageId", "ChangeRate"};
  }
  [[nodiscard]] std::vector<OGRFieldType> get_types() const override {
    return {OGRFieldType::OFTInteger, OGRFieldType::OFTInteger,
            OGRFieldType::OFTInteger, OGRFieldType::OFTReal};
  }
  [[nodiscard]] std::tuple<transect_t> get_values() const override {
    return {transect_id_, baseline_id_, image_id_, change_rate};
  }
};

struct Baseline : public MultiLine<Point<double>>, GDALShpSaver<int, int> {
  using BaselinesVertex = Point<double>;

  double transect_length_;
  double spacing_;
  double offset_;
  int baseline_id_;
  int image_id_;
  std::vector<Point<double>> transects_base_points_;
  std::vector<BaselinesVertex> baseline_vertices_;
  std::vector<TransectLine> transects_lines_;

  Baseline(const std::vector<BaselinesVertex> &points, double transect_length,
           double spacing, int baseline_id, int image_id, double offset,
           int smooth_factor, gm::IntersectionMode mode);

  [[nodiscard]] size_t size() const override {
    return transects_lines_.size();
  };

  [[nodiscard]] const Point<double> &operator[](size_t i) const override {
    return transects_lines_.at(i).transect_ref_point_;
  }

  [[nodiscard]] std::vector<std::string> get_names() const override {
    return {"BaselineId", "ImageId"};
  }

  [[nodiscard]] std::vector<OGRFieldType> get_types() const override {
    return {OGRFieldType::OFTInteger, OGRFieldType::OFTInteger};
  }

  [[nodiscard]] std::tuple<int, int> get_values() const override {
    return {baseline_id_, image_id_};
  }
};

struct Shoreline : public MultiLine<Point<double>>, GDALShpSaver<int, int> {
  std::vector<gm::Point<double>> shoreline_vertices_;  // shoreline vertices
  int shoreline_id_;                                   // shoreline id
  int year_;                                           // shoreline year
  int image_id_;

  [[nodiscard]] size_t size() const override {
    return shoreline_vertices_.size();
  }

  [[nodiscard]] const Point<double> &operator[](size_t i) const override {
    return shoreline_vertices_[i];
  }

  [[nodiscard]] std::vector<std::string> get_names() const override {
    return {"year", "ImageId"};
  }

  [[nodiscard]] std::vector<OGRFieldType> get_types() const override {
    return {OGRFieldType::OFTInteger, OGRFieldType::OFTInteger};
  }

  [[nodiscard]] std::tuple<int, int> get_values() const override {
    return {year_, image_id_};
  }
};

struct IntersectPoint : public Point<double>,
                        GDALShpSaver<int, int, int, int, int, double> {
  int image_id_;
  int transect_id_;
  int shoreline_id_;
  int baseline_id_;
  int year_;
  double distance_to_ref_;

  IntersectPoint(Point<double> point, int transect_id, int shoreline_id,
                 int baseline_id, int image_id, int year,
                 double distance_to_ref)
      : Point<double>(point),
        transect_id_(transect_id),
        shoreline_id_(shoreline_id),
        baseline_id_(baseline_id),
        image_id_(image_id),
        year_(year),
        distance_to_ref_(distance_to_ref) {}

  [[nodiscard]] std::vector<std::string> get_names() const override {
    return {"BaselineId", "TransectId", "ShoreID", "ImageID", "Year", "Dist"};
  }

  [[nodiscard]] std::vector<OGRFieldType> get_types() const override {
    return {OGRFieldType::OFTInteger, OGRFieldType::OFTInteger,
            OGRFieldType::OFTInteger, OGRFieldType::OFTInteger,
            OGRFieldType::OFTInteger, OGRFieldType::OFTReal};
  }

  [[nodiscard]] std::tuple<int, int, int, int, int, double> get_values()
      const override {
    return {baseline_id_, transect_id_, shoreline_id_,
            image_id_,    year_,        distance_to_ref_};
  }
};

}  // namespace gm

#endif  // DSAS_CPP_GEOMETRY_H