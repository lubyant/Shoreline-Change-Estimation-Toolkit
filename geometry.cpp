//
// Created by lby on 6/11/23.
//

#include "geometry.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) < (B) ? (B) : (A))
#define EPS_OFFSET 1e-6

template<typename T>
T crossProduct(std::vector<T> &vec1, std::vector<T> &vec2) {
    return vec1[0] * vec2[1] - vec1[1] * vec2[0];
}

double calCrossOfTwoVectors(const gm::Point &p1, const gm::Point &p2,
                            const gm::Point &p3, const gm::Point &p4) {
    double x1 = p2.x - p1.x;
    double x2 = p4.x - p3.x;
    double y1 = p2.y - p1.y;
    double y2 = p4.y - p3.y;
    std::vector<double> v1 = {x1, y1};
    std::vector<double> v2 = {x2, y2};

    return crossProduct(v1, v2);
}

bool testRectangularOfIntersection(const gm::Point &p1, const gm::Point &p2,
                                   const gm::Point &p3, const gm::Point &p4) {
    double l_x_min = MIN(p1.x, p2.x);
    double l_x_max = MAX(p1.x, p2.x);
    double r_x_min = MIN(p3.x, p4.x);
    double r_x_max = MAX(p3.x, p4.x);
    double l_y_min = MIN(p1.y, p2.y);
    double l_y_max = MAX(p1.y, p2.y);
    double r_y_min = MIN(p3.y, p4.y);
    double r_y_max = MIN(p3.y, p4.y);
    return (l_x_max >= r_x_min) && (r_x_max >= l_x_min) && (r_y_max >= l_y_min) &&
           (l_y_max >= r_y_min);
}

bool isTwoSegmentIntersected(const gm::Point &p1, const gm::Point &p2,
                             const gm::Point &p3, const gm::Point &p4) {
    if (testRectangularOfIntersection(p1, p2, p3, p4)) {
        if ((calCrossOfTwoVectors(p3, p1, p3, p4) *
             calCrossOfTwoVectors(p3, p2, p3, p4) <=
             0) &&
            (calCrossOfTwoVectors(p2, p3, p2, p1) *
             calCrossOfTwoVectors(p2, p4, p2, p1) <=
             0)) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

double linearRegressRate(const std::vector<double> &years,
                         const std::vector<double> &distances) {
    u_long n = years.size();
    double mean_year =
            std::accumulate(years.begin(), years.end(), 0.0) / (double) years.size();
    double mean_dis =
            std::accumulate(years.begin(), years.end(), 0.0) / (double) years.size();

    // Calculating cross-deviation and deviation of x
    double num = 0.0, den = 0.0;
    for (int i = 0; i < n; i++) {
        num += (years[i] - mean_year) * (distances[i] - mean_dis);
        den += (years[i] - mean_year) * (years[i] - mean_year);
    }
    return num / den;
}

gm::Point computeIntersectPoint(const gm::Point &p1, const gm::Point &p2,
                                const gm::Point &p3, const gm::Point &p4) {
    double x1 = p1.x, y1 = p1.y;
    double x2 = p2.x, y2 = p2.y;
    double x3 = p3.x, y3 = p3.y;
    double x4 = p4.x, y4 = p4.y;
    double a0 = y1 - y2, b0 = x2 - x1, c0 = x1 * y2 - x2 * y1;
    double a1 = y3 - y4, b1 = x4 - x3, c1 = x3 * y4 - x4 * y3;
    double d = a0 * b1 - a1 * b0;
    if (d == 0)
        return {-999999, -999999};
    else {
        double x = (b0 * c1 - b1 * c0) / d;
        double y = (c0 * a1 - c1 * a0) / d;
        return {x, y};
    }
}

int gm::Point::num_points = 0;
int gm::LineSegment::num_lines = 0;

// point
double gm::Point::distanceToPoint(const Point &point) const {
    return sqrt(pow(x - point.x, 2) + pow(y - point.y, 2));
}

bool gm::operator==(const gm::Point &point1, const gm::Point &point2) {
    return (point1.x == point2.x && point1.y == point2.y);
}

gm::Point::Point(const gm::Point &point) {
    x = point.x;
    y = point.y;
    num_points++;
}

gm::Point &gm::Point::operator=(gm::Point const &point) {
    if (this == &point)
        return *this;
    x = point.x;
    y = point.y;

    return *this;
}

gm::Point gm::Point::createPoint(double direction, double dest) const {
    double x_new = x + dest * cos(direction);
    double y_new = y + dest * sin(direction);

    return {x_new, y_new};
}

void gm::Point::movePoint(double direction, double dest) {
    x += dest * cos(direction);
    y += dest * sin(direction);
}

std::ostream &gm::operator<<(std::ostream &os, const gm::Point &point) {
    os << "x: " << point.x << ", y: " << point.y;
    return os;
}

gm::Point &gm::Point::operator=(gm::Point &&point) noexcept {
    x = point.x;
    y = point.y;
    return *this;
}

// line
bool gm::operator==(gm::LineSegment &line1, gm::LineSegment &line2) {
    return (line1.rightEdge_ == line2.rightEdge_ &&
            line1.leftEdge_ == line2.leftEdge_);
}

std::ostream &gm::operator<<(std::ostream &os,
                             const gm::LineSegment &lineSegment) {
    os << "left edge: " << lineSegment.leftEdge_
       << ", right edge: " << lineSegment.rightEdge_;
    return os;
}

gm::LineSegment::LineSegment(const gm::LineSegment &lineSegment)
        : leftEdge_(lineSegment.leftEdge_), rightEdge_(lineSegment.rightEdge_),
          slope_(lineSegment.slope_), intercept_(lineSegment.intercept_),
          orient_(lineSegment.orient_) {
    num_lines++;
}

void gm::LineSegment::moveLine(double dest) {
    if (dest != 0) {
        double normalDir = orient_;
        leftEdge_.movePoint(normalDir, dest);
        rightEdge_.movePoint(normalDir, dest);
    }
}

gm::LineSegment gm::LineSegment::createLine(double dest) const {
    if (dest != 0) {
        double normalDir = orient_;
        Point leftEdgeNew = leftEdge_.createPoint(normalDir, dest);
        Point rightEdgeNew = rightEdge_.createPoint(normalDir, dest);
        return {leftEdgeNew, rightEdgeNew};
    } else {
        return {leftEdge_, rightEdge_};
    }
}

bool gm::LineSegment::isIntersect(gm::Point &point1, gm::Point &point2) const {
    return isTwoSegmentIntersected(leftEdge_, rightEdge_, point1, point2);
}

gm::Point gm::LineSegment::findIntersection(gm::Point &point1,
                                            gm::Point &point2) const {
    if (!isIntersect(point1, point2))
        throw std::runtime_error("Not intersect");
    return computeIntersectPoint(leftEdge_, rightEdge_, point1, point2);
}

gm::LineSegment &
gm::LineSegment::operator=(const gm::LineSegment &lineSegment) {
    leftEdge_ = lineSegment.leftEdge_;
    rightEdge_ = lineSegment.rightEdge_;
    slope_ = lineSegment.slope_;
    intercept_ = lineSegment.intercept_;
    return *this;
}

gm::LineSegment::LineSegment(gm::LineSegment &&lineSegment) noexcept {
    leftEdge_ = std::move(lineSegment.leftEdge_);
    rightEdge_ = std::move(lineSegment.rightEdge_);
    slope_ = lineSegment.slope_;
    intercept_ = lineSegment.intercept_;
    orient_ = lineSegment.orient_;
    num_lines++;
}

gm::LineSegment &
gm::LineSegment::operator=(gm::LineSegment &&lineSegment) noexcept {
    leftEdge_ = std::move(lineSegment.leftEdge_);
    rightEdge_ = std::move(lineSegment.rightEdge_);
    slope_ = lineSegment.slope_;
    intercept_ = lineSegment.intercept_;
    return *this;
}

gm::LineSegment::LineSegment()
        : leftEdge_(), rightEdge_(), slope_(0), intercept_(0), orient_(0) {
    num_lines++;
}

gm::LineSegment::LineSegment(gm::Point leftEdge, gm::Point rightEdge)
        : leftEdge_(std::move(leftEdge)), rightEdge_(std::move(rightEdge)) {
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

gm::Shorelines::Shorelines(const std::vector<Point> &shore_points, int year,
                           unsigned int shoreline_id)
        : year(year),
          shore_ptr_{new std::vector<std::unique_ptr<Point>>{}}, id(shoreline_id) {
    for (const auto &point: shore_points) {
        shore_ptr_->push_back(std::make_unique<Point>(point));
    }
}

gm::Shorelines::Shorelines()
        : year{0}, id{0}, shore_ptr_{new std::vector<std::unique_ptr<Point>>()} {}

gm::Shorelines::Shorelines(const gm::Shorelines &shorelines)
        : year(shorelines.year), id(shorelines.id),
          shore_ptr_{new std::vector<std::unique_ptr<Point>>()} {
    for (unsigned int i = 0; i < shorelines.size(); i++) {
        shore_ptr_->push_back(std::make_unique<Point>(shorelines[i]));
    }
}

gm::Shorelines &gm::Shorelines::operator=(const gm::Shorelines &shorelines) {
    if (this != &shorelines) {
        year = shorelines.year;
        id = shorelines.id;
        delete shore_ptr_;

        shore_ptr_ = new std::vector<std::unique_ptr<Point>>();
        for (unsigned int i = 0; i < shorelines.size(); i++) {
            shore_ptr_->push_back(std::make_unique<Point>(shorelines[i]));
        }
    }
    return *this;
}

gm::Shorelines::Shorelines(gm::Shorelines &&shorelines) noexcept
        : year(shorelines.year), id(shorelines.id),
          shore_ptr_(shorelines.shore_ptr_) {
    shorelines.shore_ptr_ = nullptr;
}

gm::Shorelines &
gm::Shorelines::operator=(gm::Shorelines &&shorelines) noexcept {
    if (this != &shorelines) {
        id = shorelines.id;
        year = shorelines.year;
        delete shore_ptr_;
        shore_ptr_ = shorelines.shore_ptr_;
        shorelines.shore_ptr_ = nullptr;
    }
    return *this;
}

std::vector<std::unique_ptr<gm::LineSegment>>
gm::Shorelines::shores() const noexcept {
    auto ret = std::vector<std::unique_ptr<LineSegment>>();
    for (unsigned int i = 0; i < shore_ptr_->size() - 1; i++) {
        ret.push_back(
                std::make_unique<LineSegment>(operator[](i), operator[](i + 1)));
    }
    return ret;
}

gm::BaselineSeg::BaselineSeg(double spacing, double offset,
                             const Point &leftEdge, const Point &rightEdge,
                             double spacing_leftover)
        : LineSegment(leftEdge, rightEdge), spacing_(spacing), offset_(offset),
          spacing_leftover_(spacing_leftover),
          transects_ptr_(new std::vector<std::unique_ptr<Point>>()) {
    moveLine(offset_);
    const double length = leftEdge.distanceToPoint(rightEdge);
    double start = spacing_leftover_;
    double ratio = start / length;

    if (spacing_ < length - start) {
        double x_l{leftEdge.x}, y_l{leftEdge.y};
        double x_r{rightEdge.x}, y_r{rightEdge.y};
        double x_start{x_l + ratio * (x_r - x_l)};
        double y_start{y_l + ratio * (y_r - y_l)};
        gm::Point p0{x_start, y_start};
        int num = floor((p0.distanceToPoint(rightEdge) + spacing) / spacing_);
        double x_step = sqrt(spacing_ * spacing_ / (1 + slope_ * slope_)) *
                        fabs(x_r - x_l) / (x_r - x_l + EPS_OFFSET);
        double y_step =
                sqrt(slope_ * slope_ * spacing_ * spacing_ / (1 + slope_ * slope_));
        double x_cur, y_cur;
        for (int i = 0; i < num; i++) {
            x_cur = x_start + i * x_step;
            y_cur = y_start + i * y_step;
            transects_ptr_->push_back(std::make_unique<Point>(x_cur, y_cur));
        }
        spacing_leftover_ =
                spacing_ - sqrt((rightEdge.x - x_cur) * (rightEdge.x - x_cur) +
                                (rightEdge.y - y_cur) * (rightEdge.y - y_cur));
    }
}

gm::BaselineSeg::BaselineSeg(const gm::BaselineSeg &baselineSeg)
        : LineSegment(baselineSeg.leftEdge_, baselineSeg.rightEdge_),
          transects_ptr_(new std::vector<std::unique_ptr<Point>>()),
          spacing_(baselineSeg.spacing_), offset_(baselineSeg.offset_),
          spacing_leftover_(baselineSeg.spacing_leftover_) {
    for (const auto &unique_ptr: *baselineSeg.transects_ptr_) {
        transects_ptr_->push_back(std::make_unique<Point>(*unique_ptr));
    }
}

gm::BaselineSeg &
gm::BaselineSeg::operator=(const gm::BaselineSeg &baselineSeg) {
    if (this != &baselineSeg) {
        leftEdge_ = baselineSeg.leftEdge_;
        rightEdge_ = baselineSeg.rightEdge_;
        spacing_ = baselineSeg.spacing_;
        offset_ = baselineSeg.offset_;
        delete transects_ptr_;
        transects_ptr_ = new std::vector<std::unique_ptr<Point>>();
        for (const auto &unique_ptr: *baselineSeg.transects_ptr_) {
            transects_ptr_->push_back(std::make_unique<Point>(*unique_ptr));
        }
    }
    return *this;
}

gm::BaselineSeg::BaselineSeg(gm::BaselineSeg &&baselineSeg) noexcept
        : spacing_(baselineSeg.spacing_), offset_(baselineSeg.offset_),
          spacing_leftover_(baselineSeg.spacing_leftover_),
          transects_ptr_(baselineSeg.transects_ptr_) {
    baselineSeg.transects_ptr_ = nullptr;
}

gm::BaselineSeg &
gm::BaselineSeg::operator=(gm::BaselineSeg &&baselineSeq) noexcept {
    if (this != &baselineSeq) {
        spacing_ = baselineSeq.spacing_;
        offset_ = baselineSeq.offset_;
        spacing_leftover_ = baselineSeq.spacing_leftover_;
        delete transects_ptr_;
        transects_ptr_ = baselineSeq.transects_ptr_;
        baselineSeq.transects_ptr_ = nullptr;
    }
    return *this;
}

gm::Baselines::Baselines(const Shorelines &shorelines, double transect_length,
                         double spacing, int baseline_id, double offset,
                         int smooth_factor)
        : baseline_id(baseline_id), spacing_(spacing),
          transect_length_(transect_length),
          baselines_(new std::vector<std::unique_ptr<BaselineSeg>>()),
          offset_(offset), transects_(new std::vector<std::unique_ptr<Point>>()),
          transects_line_(new std::vector<std::unique_ptr<TransectLine>>()) {
    // create the baseline by shorelines (smoothing)
    double spacing_leftover = 0;
    ulong num_shores = shorelines.size();
    int transect_id = 0;
    for (ulong i = 0; i < num_shores; i += smooth_factor) {
        BaselineSeg baselineSeg{spacing_, offset_, shorelines[i],
                                shorelines[i + smooth_factor], spacing_leftover};
        spacing_leftover = baselineSeg.spacing_leftover_;
        for (const auto &transect: *baselineSeg.transects_ptr_) {
            transects_->insert(transects_->end(), std::make_unique<Point>(*transect));
            transects_line_->push_back(std::make_unique<TransectLine>(
                    *transect, baselineSeg.orient_, transect_length_, transect_id++));
        }
        baselines_->push_back(std::make_unique<BaselineSeg>(
                spacing_, offset_, shorelines[i], shorelines[i + smooth_factor],
                spacing_leftover));
    }
}

gm::Baselines::Baselines(const gm::Baselines &baselines)
        : spacing_(baselines.spacing_), baseline_id(baselines.baseline_id),
          transect_length_(baselines.transect_length_), offset_(baselines.offset_),
          transects_(new std::vector<std::unique_ptr<Point>>()),
          transects_line_(new std::vector<std::unique_ptr<TransectLine>>()),
          baselines_(new std::vector<std::unique_ptr<BaselineSeg>>()) {
    for (const auto &transect: *baselines.transects_) {
        transects_->push_back(std::make_unique<Point>(*transect));
    }

    for (const auto &transect_line: *baselines.transects_line_) {
        transects_line_->push_back(std::make_unique<TransectLine>(*transect_line));
    }

    for (const auto &baseline_seg: *baselines.baselines_) {
        baselines_->push_back(std::make_unique<BaselineSeg>(*baseline_seg));
    }
}

gm::Baselines &gm::Baselines::operator=(const gm::Baselines &baselines) {
    if (this != &baselines) {
        spacing_ = baselines.spacing_;
        transects_line_ = baselines.transects_line_;
        offset_ = baselines.offset_;
        delete transects_;
        transects_ = new std::vector<std::unique_ptr<Point>>();
        delete transects_line_;
        transects_line_ = new std::vector<std::unique_ptr<TransectLine>>();
        for (const auto &transect: *baselines.transects_) {
            transects_->push_back(std::make_unique<Point>(*transect));
        }

        for (const auto &transect_line: *baselines.transects_line_) {
            transects_line_->push_back(
                    std::make_unique<TransectLine>(*transect_line));
        }
    }
    return *this;
}

gm::Baselines::Baselines(gm::Baselines &&baselines) noexcept
        : transect_length_(baselines.transect_length_),
          spacing_(baselines.spacing_), offset_(baselines.offset_),
          baseline_id(baselines.baseline_id), transects_(baselines.transects_),
          baselines_(baselines.baselines_),
          transects_line_(baselines.transects_line_) {
    baselines.transects_ = nullptr;
    baselines.baselines_ = nullptr;
    baselines.transects_line_ = nullptr;
}

gm::Baselines &gm::Baselines::operator=(gm::Baselines &&baselines) noexcept {
    if (this != &baselines) {
        transects_ = baselines.transects_;
        spacing_ = baselines.spacing_;
        offset_ = baselines.offset_;

        // move the transects
        delete transects_;
        transects_ = baselines.transects_;
        baselines.transects_ = nullptr;

        // move the baselines
        delete baselines_;
        baselines_ = baselines.baselines_;
        baselines.baselines_ = nullptr;

        // move the transect_line_
        delete transects_line_;
        transects_line_ = baselines.transects_line_;
        baselines.transects_line_ = nullptr;
    }
    return *this;
}

std::vector<std::unique_ptr<gm::Point>>
gm::Baselines::intersect_shorelines(gm::Shorelines &shorelines) const {
    std::vector<std::unique_ptr<Point>> intersects{};
    for (const auto &transect: *transects_line_) {
        for (const auto &shore_seg: shorelines.shores()) {
            if (transect->isIntersect(shore_seg->leftEdge_, shore_seg->rightEdge_)) {
                intersects.push_back(std::make_unique<Point>(transect->findIntersection(
                        shore_seg->leftEdge_, shore_seg->rightEdge_)));
                break;
            }
        }
    }
    return intersects;
}

gm::TransectLine::TransectLine(Point &transect_base, double transect_length,
                               double baseline_orient, int transect_id)
        : LineSegment(std::move(
        create_transect(transect_base, baseline_orient, transect_length))),
          transect_base_(transect_base), transect_orient_(baseline_orient + PI / 2),
          transect_length_(transect_length), baseline_orient_(baseline_orient),
          transect_id_(transect_id) {}

gm::LineSegment gm::TransectLine::create_transect(gm::Point &transect_base,
                                                  double baseline_orient,
                                                  double transect_length) {
    gm::Point leftEdge = std::move(
            transect_base.createPoint(baseline_orient - PI / 2, transect_length / 2));
    gm::Point rightEdge = std::move(
            transect_base.createPoint(baseline_orient + PI / 2, transect_length / 2));

    return {leftEdge, rightEdge};
}

void gm::Intersections::reg_rate() {
    assert(!year_distance_map_->empty());
    std::vector<std::vector<double>> tbl{};
    std::vector<double> years;
    std::vector<double> distances;
    double distance;
    for (const auto &pair: *year_distance_map_) {
        years.push_back((double) pair.first);
        distance = pair.second.second;
        std::vector<double> temp = {(double) pair.first, distance};
        distances.push_back(distance);
        tbl.emplace_back(temp);
    }

    // sort
    if (tbl.size() > 1) {
        std::sort(tbl.begin(), tbl.end(),
                  [](const auto &a, const auto &b) { return a[0] < b[0]; });
    }

    // compute the end point rate
    epr = (tbl.end()->at(1) - tbl.begin()->at(1)) /
          (tbl.begin()->at(0) - tbl.begin()->at(0));

    // compute the linear regression rate
    lrr = linearRegressRate(years, distances);
}

gm::Intersections::Intersections(
        int baselineId, const std::vector<std::vector<Shorelines>> &shores_inv,
        const gm::TransectLine &transectLine)
        : baseline_id_(baselineId), transect_id_(transectLine.transect_id_),
          transectLine_(transectLine),
          year_distance_map_(
                  new std::unordered_map<int, std::pair<Point, double>>()) {
    // loop the year
    int year;
    for (const auto &shores: shores_inv) {
        for (const auto &shore: shores) {
            year = shore.year;
            for (const auto &shore_seg: shore.shores()) {
                if (transectLine.isIntersect(shore_seg->leftEdge_,
                                             shore_seg->rightEdge_)) {
                    Point &&intersect{transectLine.findIntersection(
                            shore_seg->leftEdge_, shore_seg->rightEdge_)};
                    if (!isYearExist(year)) {
                        (*year_distance_map_)[year] = {
                                intersect, intersect.distanceToPoint(shore_seg->leftEdge_)};
                    } else {
                        if (intersect.distanceToPoint(shore_seg->leftEdge_) <
                            year_distance_map_->at(year).second) {
                            year_distance_map_->at(year) = {
                                    intersect, intersect.distanceToPoint(shore_seg->leftEdge_)};
                        }
                    }
                }
            }
        }
    }
    reg_rate();
}
