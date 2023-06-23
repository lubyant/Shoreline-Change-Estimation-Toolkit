//
// Created by lby on 6/11/23.
//

#ifndef SHORECALCULATOR_GEOMETRY_H
#define SHORECALCULATOR_GEOMETRY_H

#include <iostream>
#include <utility>
#include <vector>
#include <memory>
#include <cmath>
#define EPS_OFFSET 1e-6
#define PI 3.14159265


namespace gm {
    class Point {
    public:
        double x, y;
        int static num_points;

        // constructor
        Point() : x(0.0), y(0.0) { num_points++; }

        Point(const double x, const double y) : x(x), y(y) { num_points++; }

        // copy constructor
        Point(const Point &point);

        // destructor
        ~Point() { num_points--; }

        // copy assignment
        Point &operator=(Point const &point);

        // move constructor
        Point(Point &&other) noexcept: x(other.x), y(other.y) {}

        // move assignment
        Point &operator=(Point &&point) noexcept;

        // overload
        friend bool operator==(const Point &point1, const Point &point2);

        friend std::ostream &operator<<(std::ostream &os, const Point &point);

        // class method
        [[nodiscard]] double distanceToPoint(const Point &point) const;

        [[nodiscard]] Point createPoint(double direction, double dest) const;

        void movePoint(double direction, double dest);

    };

    class LineSegment {
    public:
        Point leftEdge, rightEdge;
        double slope = 0, intercept = 0, orient = 0;
        static int num_lines;

        // constructor
        LineSegment() = default;

        LineSegment(Point leftEdge, Point rightEdge) : leftEdge(std::move(leftEdge)),
                                                       rightEdge(std::move(rightEdge)) {
            num_lines++;
            double dx = rightEdge.x - leftEdge.x, dy = rightEdge.y - leftEdge.y;
            slope = dy/(dx + EPS_OFFSET);
            intercept = this->leftEdge.y - slope * this->leftEdge.x;
            if(dy >= 0){
                if(dx >= 0){
                    orient = atan(fabs(slope));
                } else {
                    orient = atan(fabs(slope)) + PI/2;
                }
            } else {
                if (dx >= 0){
                    orient = 2*PI - atan(fabs(slope));
                } else {
                    orient = PI + atan(fabs(slope));
                }
            }
        }

        // copy constructor
        LineSegment(const LineSegment &lineSegment);

        // move constructor
        LineSegment(LineSegment &&lineSegment) noexcept;

        // copy assignment
        LineSegment &operator=(const LineSegment &lineSegment);

        // move assignment
        LineSegment &operator=(LineSegment &&lineSegment) noexcept;

        // destructor
        ~LineSegment() { num_lines--; }

        // overload
        friend bool operator==(LineSegment &line1, LineSegment &line2);

        friend std::ostream &operator<<(std::ostream &os, const LineSegment &lineSegment);

        // class method
        void moveLine(double dest);

        [[nodiscard]] LineSegment createLine(double dest) const;

        bool isIntersect(Point &point1, Point &point2) const;

        Point findIntersection(Point &point1, Point &point2) const;
    };

    class BaselineSeg : public LineSegment {
    public:
        double spacing_, offset_, spacing_leftover_;
        std::vector<Point> *transects_ptr_;
    public:
        BaselineSeg(double spacing, double offset, const Point &leftEdge, const Point &rightEdge,
                    double spacing_leftover);

        BaselineSeg() = default;

        ~BaselineSeg() {delete transects_ptr_;}
    };

    class MultiLines {
    public:
        unsigned int id = 0;
        std::vector<LineSegment> *lines_vec_ptr_{};
        [[nodiscard]] unsigned long size() const{return lines_vec_ptr_->size(); }
    public:
        MultiLines() = default;

        explicit MultiLines(std::vector<Point> &points, unsigned int id = 0);

        MultiLines(const MultiLines &multiLines);

        MultiLines& operator=(const MultiLines &multiLines);


        MultiLines(MultiLines &&multiLines) noexcept;


        ~MultiLines() {delete lines_vec_ptr_;}

        const LineSegment &operator[](unsigned int i) const {
            return (*lines_vec_ptr_)[i];
        }

        [[nodiscard]] unsigned int getId() const { return id; }
    };

    class Shorelines : public MultiLines {
    private:
        std::string year;
    public:
        Shorelines() = default;

        Shorelines(std::vector<Point> &shore_points, std::string &year, unsigned int shoreline_id = 0);

        ~Shorelines() = default;

        void pushBack(LineSegment line);

        void pushFront(LineSegment line);
    };

    class TransectLine: LineSegment{
    private:
        Point transect_base_;
        LineSegment transect_line_;
        double transect_length_, baseline_orient;

        static LineSegment create_transect(gm::Point &transect_base, double baseline_orient, double transect_length);

    public:
        TransectLine(Point &transect_base, double transect_length, double baseline_orient);

        TransectLine(TransectLine & transectLine) = default;

        TransectLine(TransectLine &&transectLine) = default;

    };

    class Baselines : public MultiLines {
    protected:
        const double transect_length_;
        const double spacing_;
        const double offset_;
    public:
        std::vector<Point> *transects_;
        std::vector<TransectLine> *transects_line_;
        Baselines(std::vector<Point> &baseline_points, double transect_length, double spacing,
                  int baseline_id, double offset);
        ~Baselines() {delete transects_; delete transects_line_;}
    };



}


#endif //SHORECALCULATOR_GEOMETRY_H
