//
// Created by lby on 10/20/23.
//

#ifndef SHORELINECALCULATOR_GEOMETRY_HPP
#define SHORELINECALCULATOR_GEOMETRY_HPP

#define EPS_OFFSET 1e-6
#define PI 3.1415926

#include <gdal_priv.h>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "options.hpp"

// classes
namespace gm {

template <typename T = double>
struct Point;

struct Shoreline;
using Shorelines = std::vector<gm::Shoreline>;
struct IntersectPoint;
struct Transects;

template <typename T>
std::ostream &operator<<(std::ostream &os, const Point<T> &point);

template <typename T>
struct MultiLine {
  [[nodiscard]] virtual size_t size() const = 0;

  [[nodiscard]] virtual const T &operator[](size_t i) const = 0;

  virtual ~MultiLine() = default;
};

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
  size_t id_{};

  Point() : x(0), y(0) {};
  Point(T x, T y) : x(x), y(y) {}
  Point(T x, T y, size_t id) : x(x), y(y), id_(id) {}

  Point(const Point &point) = default;
  Point(Point &&point) noexcept = default;

  Point &operator=(const Point &point) = default;
  Point &operator=(Point &&point) = default;

  friend std::ostream &operator<< <T>(std::ostream &os, const Point<T> &point);
  friend bool operator==(const Point<T> &point1, const Point<T> &point2) {
    return (point1.x == point2.x) && (point1.y == point2.y);
  }
  friend bool operator!=(const Point<T> &point1, const Point<T> &point2) {
    return (point1.x != point2.x) || (point1.y != point2.y);
  }

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
    if (orient.first == 0 && orient.second == 0) {
      return {x, y};
    }
    double dist =
        sqrt(orient.first * orient.first + orient.second * orient.second);
    double y_new = y + dest * orient.first / dist;
    double x_new = x + dest * orient.second / dist;
    return {x_new, y_new};
  }
  // Linear interpolation between two points
  static Point<T> interpolate(const Point<T> &a, const Point<T> &b,
                              double fraction) {
    return Point(a.x + (b.x - a.x) * fraction, a.y + (b.y - a.y) * fraction);
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

#define transect_t \
  int, int, int, int, double, int, const char *, const char *, const char *
struct TransectLine : public LineSegment,
                      MultiLine<Point<>>,
                      GDALShpSaver<transect_t> {
  using IntersectionMode = dsas::Options::IntersectionMode;
  using TransectOrientation = dsas::Options::TransectOrientation;
  Point<> transect_base_point_;  // point to generate the shapefile
  Point<> transect_ref_point_;   // point to calculate the erosion
  int transect_id_;
  int baseline_id_;
  int image_id_;
  int group_id_;
  size_t num_intersect_{};  // number of intersections in this transect
  double change_rate{};     // change rate for all the intersections
  IntersectionMode mode_;
  TransectOrientation orient_;
  std::unordered_map<int, IntersectPoint *> year_intersect_map_;
  std::vector<Shoreline> shoreline_segs_;  // the shoreline segments nearby
  std::vector<double> frechet_dist_;
  std::vector<double> euc_dist_;
  std::string frechet_info_{};
  std::string euc_info_{};
  std::string intersect_info_{};  // year, dist; year, dist;....

  TransectLine *prev_transect_line{nullptr}, *next_transect_line{nullptr};

  TransectLine(Point<> &transect_base, double transect_length,
               std::pair<double, double> baseline_normal_vector,
               int transect_id, int baseline_id, int image_id,
               IntersectionMode mode = IntersectionMode::Closest,
               TransectOrientation orient = TransectOrientation::Mix)
        :LineSegment(create_transect(transect_base, baseline_normal_vector,
                                    transect_length, orient)),

        transect_base_point_(transect_base),
        transect_ref_point_(orient == TransectOrientation::Right ? leftEdge_
                                                                 : rightEdge_),
        transect_id_(transect_id),
        baseline_id_(baseline_id),
        image_id_(image_id),
        mode_(mode),
        orient_(orient) {
    if (std::isnan(transect_ref_point_.x) ||
        std::isnan(transect_ref_point_.y)) {
      throw std::runtime_error("ref point error");
    }
  }

  void truncate_shoreline_seg();
  void compute_frechet_dist();

  static LineSegment create_transect(
      Point<> &transect_base, std::pair<double, double> baseline_normal_vector,
      double transect_length, TransectOrientation orient);

  [[nodiscard]] std::optional<IntersectPoint> intersection(
      const Shoreline &shoreline) const;

  double distance2ref(Point<> &point) const {
    return transect_ref_point_.distance_to_point(point);
  }

  void set_info(const std::vector<double> &years,
                const std::vector<double> &distances);

  void set_frechet_info(int year_start, int year_end, double frechet_dist);
  void set_euc_info(int year_start, int year_end, double euc_dist);

  [[nodiscard]] size_t size() const override { return 3; }

  [[nodiscard]] const Point<> &operator[](size_t i) const override {
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
    return {"TransectId", "BaselineId", "ImageId", "GroupId", "ChangeRate",
            "Nums",       "IntInfo",    "EucInfo", "FreInfo"};
  }
  [[nodiscard]] std::vector<OGRFieldType> get_types() const override {
    return {OGRFieldType::OFTInteger, OGRFieldType::OFTInteger,
            OGRFieldType::OFTInteger, OGRFieldType::OFTInteger,
            OGRFieldType::OFTReal,    OGRFieldType::OFTInteger,
            OGRFieldType::OFTString,  OGRFieldType::OFTString,
            OGRFieldType::OFTString};
  }
  [[nodiscard]] std::tuple<transect_t> get_values() const override {
    return {transect_id_,
            baseline_id_,
            image_id_,
            group_id_,
            change_rate,
            num_intersect_,
            intersect_info_.c_str(),
            euc_info_.c_str(),
            frechet_info_.c_str()};
  }
};

struct Transects {
  int baseline_id_;
  std::vector<TransectLine> transects_;
};

struct Baseline : public MultiLine<Point<>>, GDALShpSaver<int, int> {
  using BaselinesVertex = Point<>;

  int baseline_id_;
  int image_id_;
  std::vector<Point<>> transects_base_points_;
  std::vector<BaselinesVertex>
      origin_vertices_;  // the shoreline vertices for generate baseline
  std::vector<BaselinesVertex>
      baseline_vertices_;  // final baseline vertices for compute the rate
  std::vector<std::pair<double, double>> normal_vectors_;

  const dsas::Options &options_;

  Baseline(const std::vector<BaselinesVertex> &points, int baseline_id,
           int image_id, const dsas::Options &options);

  [[nodiscard]] size_t size() const override {
    assert(baseline_vertices_.size() > 1);
    return baseline_vertices_.size();
  };

  [[nodiscard]] const Point<> &operator[](size_t i) const override {
    assert(baseline_vertices_.size() > 1);
    return baseline_vertices_.at(i);
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

  Transects set_transects() { return transects_; }

 private:
  void create_transects();
  Transects transects_;
};

struct Shoreline : public MultiLine<Point<>>, GDALShpSaver<int, int> {
  std::vector<gm::Point<double>> shoreline_vertices_;  // shoreline vertices
  int shoreline_id_{};                                 // shoreline id
  int year_{};                                         // shoreline year
  int image_id_{};
  boost::gregorian::date date_{};

  Shoreline(std::vector<gm::Point<>> &shoreline_vertices, int shoreline_id,
            int year, int image_id);

  Shoreline(std::vector<gm::Point<>> &shoreline_vertices, int shoreline_id,
            boost::gregorian::date date, int image_id);

  Shoreline() = default;

  inline friend bool operator==(const Shoreline &a, const Shoreline &b) {
    return (a.image_id_ == b.image_id_) && (a.year_ == b.year_) &&
           (a.shoreline_id_ == b.shoreline_id_);
  }

  [[nodiscard]] size_t size() const override {
    return shoreline_vertices_.size();
  }

  [[nodiscard]] const Point<> &operator[](size_t i) const override {
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

#define IntersectPoint_t \
  int, int, int, int, int, int, double, double, double, double
struct IntersectPoint : public Point<>, GDALShpSaver<IntersectPoint_t> {
  int image_id_;
  int transect_id_;
  int shoreline_id_;
  int baseline_id_;
  int group_id_;
  int year_;
  boost::gregorian::date date_;
  double distance_to_ref_{-1};
  double euc_distance_diff{-1};
  double frechet_distance_diff_{-1};  // the frechet distance difference between
                                      // year[i-1], year[i], year[i+1]
  const TransectLine *transect_line_ptr_{nullptr};
  const Shoreline *shoreline_ptr_{nullptr};  // the shoreline intersect stands
  bool is_fre_outlier{false};
  bool is_base_outlier{false};
  bool is_outlier{false};

  IntersectPoint(Point<double> point, int transect_id, int shoreline_id,
                 int baseline_id, int image_id, int group_id, int year,
                 double distance_to_ref, const TransectLine *transect_line_ptr,
                 const Shoreline *shoreline_ptr)
      : Point<double>(point),
        image_id_(image_id),
        transect_id_(transect_id),
        shoreline_id_(shoreline_id),
        baseline_id_(baseline_id),
        group_id_(group_id),
        year_(year),
        date_(year_, 1, 1),
        distance_to_ref_(distance_to_ref),
        transect_line_ptr_(transect_line_ptr),
        shoreline_ptr_(shoreline_ptr) {}

  [[nodiscard]] std::vector<std::string> get_names() const override {
    return {"BaselineId", "TransectId", "ShoreID",  "ImageID", "GroupID",
            "Year",       "euc_dist",   "fre_dist", "X",       "Y"};
  }

  [[nodiscard]] std::vector<OGRFieldType> get_types() const override {
    return {OGRFieldType::OFTInteger, OGRFieldType::OFTInteger,
            OGRFieldType::OFTInteger, OGRFieldType::OFTInteger,
            OGRFieldType::OFTInteger, OGRFieldType::OFTInteger,
            OGRFieldType::OFTReal,    OGRFieldType::OFTReal,
            OGRFieldType::OFTReal,    OGRFieldType::OFTReal};
  }

  [[nodiscard]] std::tuple<IntersectPoint_t> get_values() const override {
    return {baseline_id_,
            transect_id_,
            shoreline_id_,
            image_id_,
            group_id_,
            year_,
            euc_distance_diff,
            frechet_distance_diff_,
            x,
            y};
  }
};

using Path = std::filesystem::path;
using Baselines = std::vector<Baseline>;
using TransectGroups = std::vector<Transects>;

}  // namespace gm

#endif  // SHORELINECALCULATOR_GEOMETRY_HPP