//
// Created by lby on 6/11/23.
//

#ifndef SHORECALCULATOR_GEOMETRY_H
#define SHORECALCULATOR_GEOMETRY_H

#include <iostream>
#include <utility>
#include <vector>
#include <memory>


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
        double slope = 0, intercept = 0;
        static int num_lines;

        // constructor
        LineSegment() = default;

        LineSegment(Point leftEdge, Point rightEdge) : leftEdge(std::move(leftEdge)),
                                                       rightEdge(std::move(rightEdge)) {
            num_lines++;
            slope = (this->leftEdge.y - this->rightEdge.y) / (this->leftEdge.x - this->rightEdge.x);
            intercept = this->leftEdge.y - slope * this->leftEdge.x;
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
        std::unique_ptr<std::vector<Point>> transects_ptr_;
    public:
        BaselineSeg(double spacing, double offset, const Point &leftEdge, const Point &rightEdge,
                    double spacing_leftover);

        BaselineSeg() = default;
    };

    class MultiLines {
    protected:
        const unsigned int id = 0;
        std::unique_ptr<std::vector<LineSegment>> lines_vec_ptr_;
    public:
        MultiLines() = default;

        explicit MultiLines(std::vector<Point> &points, unsigned int id = 0);

        MultiLines(const MultiLines &multiLines);

        MultiLines& operator=(const MultiLines &multiLines) = delete;

        MultiLines(MultiLines &&multiLines) noexcept;

        MultiLines& operator=(MultiLines &&multiLines) = delete;

        ~MultiLines() = default;

        const LineSegment &operator[](unsigned int i) const {
            return (*lines_vec_ptr_)[i];
        }

        [[nodiscard]] unsigned int getId() const { return id; }
    };

    class Shorelines : public MultiLines {
    private:
        const std::string year;
    public:
        Shorelines() = default;

        Shorelines(std::vector<Point> &shore_points, std::string &year, unsigned int shoreline_id = 0);

        ~Shorelines() = default;

        void pushBack(Point &point);

        void pushFront(Point &point);
    };

    class Baselines : public MultiLines {
    protected:
        std::unique_ptr<std::vector<Point>> transects_;
        const double transect_length_;
        const double spacing_;
        const double offset_;
    public:

        Baselines(const std::unique_ptr<std::vector<Point>> &baseline_points, double transect_length, double spacing,
                  int baseline_id, double offset);
    };

    class TransectLine: LineSegment{
    private:
        Point transect_base_;
        LineSegment transect_line_;
        double transect_length_;

        std::tuple<Point, Point> create_transect(Point &transect_base);

    public:
        TransectLine(Point &transect_base, double transect_length);
    };
}


#endif //SHORECALCULATOR_GEOMETRY_H
