//
// Created by lby on 6/11/23.
//

#include "geometry.h"
#include <cmath>
#include <utility>
#include <vector>

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) < (B) ? (B) : (A))
#define PI 3.1415926

template<typename T>
T crossProduct(std::vector<T> &vec1, std::vector<T> &vec2) {
    return vec1[0] * vec2[1] - vec1[1] * vec2[0];
}

double calCrossOfTwoVectors(const gm::Point &p1, const gm::Point &p2, const gm::Point &p3, const gm::Point &p4) {
    double x1 = p2.x - p1.x;
    double x2 = p4.x - p3.x;
    double y1 = p2.y - p1.y;
    double y2 = p4.y - p3.y;
    std::vector<double> v1 = {x1, y1};
    std::vector<double> v2 = {x2, y2};

    return crossProduct(v1, v2);
}

bool testRectangularOfIntersection(const gm::Point &p1, const gm::Point &p2, const gm::Point &p3, const gm::Point &p4) {
    double l_x_min = MIN(p1.x, p2.x);
    double l_x_max = MAX(p1.x, p2.x);
    double r_x_min = MIN(p3.x, p4.x);
    double r_x_max = MAX(p3.x, p4.x);
    double l_y_min = MIN(p1.y, p2.y);
    double l_y_max = MAX(p1.y, p2.y);
    double r_y_min = MIN(p3.y, p4.y);
    double r_y_max = MIN(p3.y, p4.y);
    return (l_x_max >= r_x_min) && (r_x_max >= l_x_min) && (r_y_max >= l_y_min) && (l_y_max >= r_y_min);
}

bool isTwoSegmentIntersected(const gm::Point &p1, const gm::Point &p2, const gm::Point &p3, const gm::Point &p4) {
    if (testRectangularOfIntersection(p1, p2, p3, p4)) {
        if ((calCrossOfTwoVectors(p3, p1, p3, p4) * calCrossOfTwoVectors(p3, p2, p3, p4) <= 0) &&
            (calCrossOfTwoVectors(p2, p3, p2, p1) * calCrossOfTwoVectors(p2, p4, p2, p1) <= 0)) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

gm::Point computeIntersectPoint(const gm::Point &p1, const gm::Point &p2, const gm::Point &p3, const gm::Point &p4) {
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
    x = x + dest * cos(direction);
    y = y + dest * sin(direction);
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
    return (line1.rightEdge == line2.rightEdge && line1.leftEdge == line2.leftEdge);
}

std::ostream &gm::operator<<(std::ostream &os, const gm::LineSegment &lineSegment) {
    os << "left edge: " << lineSegment.leftEdge << ", right edge: "
       << lineSegment.rightEdge;
    return os;
}


gm::LineSegment::LineSegment(const gm::LineSegment &lineSegment) :
        leftEdge(lineSegment.leftEdge), rightEdge(lineSegment.rightEdge), slope(lineSegment.slope),
        intercept(lineSegment.intercept) {
    num_lines++;
}

void gm::LineSegment::moveLine(double dest) {
    if (dest != 0) {
        double normalDir = atan(slope) + PI / 2;
        leftEdge.movePoint(normalDir, dest);
        rightEdge.movePoint(normalDir, dest);
    }
}

gm::LineSegment gm::LineSegment::createLine(double dest) const {
    if (dest != 0) {
        double normalDir = atan(slope) + PI / 2;
        Point leftEdgeNew = leftEdge.createPoint(normalDir, dest);
        Point rightEdgeNew = rightEdge.createPoint(normalDir, dest);
        return {leftEdgeNew, rightEdgeNew};
    } else {
        return {leftEdge, rightEdge};
    }
}

bool gm::LineSegment::isIntersect(gm::Point &point1, gm::Point &point2) const {
    return isTwoSegmentIntersected(leftEdge, rightEdge, point1, point2);
}

gm::Point gm::LineSegment::findIntersection(gm::Point &point1, gm::Point &point2) const {
    if (!isIntersect(point1, point2))
        throw std::runtime_error("Not intersect");
    return computeIntersectPoint(leftEdge, rightEdge, point1, point2);
}

gm::LineSegment &gm::LineSegment::operator=(const gm::LineSegment &lineSegment) {
    leftEdge = lineSegment.leftEdge;
    rightEdge = lineSegment.rightEdge;
    slope = lineSegment.slope;
    intercept = lineSegment.intercept;
    return *this;
}

gm::LineSegment::LineSegment(gm::LineSegment &&lineSegment) noexcept {
    leftEdge = lineSegment.leftEdge;
    rightEdge = lineSegment.rightEdge;
    slope = lineSegment.slope;
    intercept = lineSegment.intercept;
}

gm::LineSegment &gm::LineSegment::operator=(gm::LineSegment &&lineSegment) noexcept {
    leftEdge = std::move(lineSegment.leftEdge);
    rightEdge = std::move(lineSegment.rightEdge);
    slope = lineSegment.slope;
    intercept = lineSegment.intercept;
    return *this;
}


gm::MultiLines::MultiLines(std::vector<Point> &points, unsigned int id) : id(id) {
    for (int i = 0; i < points.size() - 1; i++) {
        lines_vec_ptr_->push_back(LineSegment(points[i], points[i + 1]));
    }
}

gm::MultiLines::MultiLines(const gm::MultiLines &multiLines) :
        id(multiLines.id) {
    for (int i = 0; i < multiLines.lines_vec_ptr_->size() - 1; i++) {
        lines_vec_ptr_->push_back(
                LineSegment(multiLines.lines_vec_ptr_->at(i).leftEdge, multiLines.lines_vec_ptr_->at(i + 1).rightEdge));
    }
}

gm::MultiLines::MultiLines(gm::MultiLines &&multiLines) noexcept: id(multiLines.id),
                                                                  lines_vec_ptr_(std::move(multiLines.lines_vec_ptr_)) {

}


gm::Shorelines::Shorelines(std::vector<Point> &shore_points, std::string &year, unsigned int shoreline_id) :
        MultiLines(shore_points, shoreline_id), year(year) {

}

void gm::Shorelines::pushBack(gm::Point &point) {
    unsigned long lastIndex = lines_vec_ptr_->size() - 1;
    lines_vec_ptr_->emplace_back((*lines_vec_ptr_)[lastIndex].rightEdge, point);
}

void gm::Shorelines::pushFront(gm::Point &point) {
    LineSegment newShore(point, (*lines_vec_ptr_)[0].leftEdge);
    lines_vec_ptr_->insert(lines_vec_ptr_->begin(), newShore);
}


gm::BaselineSeg::BaselineSeg(double spacing, double offset, const Point &leftEdge, const Point &rightEdge,
                             double spacing_leftover) :
        LineSegment(leftEdge, rightEdge), spacing_(spacing), offset_(offset), spacing_leftover_(spacing_leftover) {
    moveLine(offset_);
    const double length = leftEdge.distanceToPoint(rightEdge);
    double start = spacing_leftover_;
    double ratio = start / length;

    if (spacing_ < length - start) {
        double x_l{leftEdge.x}, y_l{rightEdge.y};
        double x_r{rightEdge.x}, y_r{rightEdge.y};
        double x_start{x_l + ratio * (x_r - x_l)};
        double y_start{y_l + ratio * (y_r - y_l)};
        gm::Point p0{x_start, y_start};
        int num = floor(p0.distanceToPoint(rightEdge) / spacing_);
        double x_step = sqrt(spacing_ * spacing_ / (1 + slope * slope)) * fabs(x_r - x_l) / (x_r - x_l);
        double y_step = sqrt(slope * slope * spacing_ * spacing_ / (1 + slope * slope));
        double x_cur, y_cur;
        for (int i = 0; i < num; i++) {
            x_cur = x_start + i * x_step;
            y_cur = y_start + i * y_step;
            transects_ptr_->push_back(Point(x_cur, y_cur));
        }
        spacing_leftover_ = spacing_ - sqrt((rightEdge.x - x_cur) * (rightEdge.x - x_cur) +
                                            (rightEdge.y - y_cur) * (rightEdge.y - y_cur));

    }
}

gm::Baselines::Baselines(const std::unique_ptr<std::vector<Point>> &baseline_points, double transect_length, double spacing,
                         int baseline_id, const double offset) :
        MultiLines(*baseline_points, baseline_id), spacing_(spacing), transect_length_(transect_length),
        offset_(offset) {
    BaselineSeg base0{spacing_, offset_, lines_vec_ptr_->at(0).leftEdge, lines_vec_ptr_->at(0).rightEdge, 0.00};
    transects_->insert(transects_->end(), base0.transects_ptr_->begin(), base0.transects_ptr_->end());
    double left_over = base0.spacing_leftover_;

    for (int i = 1; i < lines_vec_ptr_->size(); i++) {
        BaselineSeg baselineSeg{spacing_, offset_, lines_vec_ptr_->at(i).leftEdge, lines_vec_ptr_->at(i).rightEdge,
                                left_over};
        transects_->insert(transects_->end(), baselineSeg.transects_ptr_->begin(), baselineSeg.transects_ptr_->end());
        left_over = baselineSeg.spacing_leftover_;
    }
}

gm::TransectLine::TransectLine(gm::Point &transect_base, double transect_length) :
        transect_base_(transect_base), transect_length_(transect_length)
{

}

std::tuple<gm::Point, gm::Point> gm::TransectLine::create_transect(gm::Point &transect_base) {
    gm::Point leftEdge, rightEdge;

    return std::tuple<Point, Point>();
}
