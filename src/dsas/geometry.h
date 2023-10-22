//
// Created by lby on 10/20/23.
//

#ifndef DSAS_CPP_GEOMETRY_H
#define DSAS_CPP_GEOMETRY_H

#define EPS_OFFSET 1e-6
#define PI 3.1415926

#include <cmath>
#include <iostream>
#include <vector>


// classes
namespace gm {

    template<typename T = double>
    struct Point;

    template<typename T>
    std::ostream &operator<<(std::ostream &os, const Point<T> &point);

    template<typename T>
    struct MultiLine {
        [[nodiscard]] virtual const size_t size() const = 0;

        [[nodiscard]] virtual const T &operator[](size_t i) const = 0;
    };


    template<typename T>
    struct Point {
        T x, y;

        Point(T x, T y) : x(x), y(y) {}

        friend std::ostream &operator
        <<<T>(
        std::ostream &os,
        const Point<T> &point
        );

        [[nodiscard]] T distance_to_point(const Point<T> &point) const {
            return sqrt(pow(x - point.x, 2) + pow(y - point.y, 2));
        }

        void move_point(double direction, T dest) {
            x += dest * cos(direction);
            y += dest * sin(direction);
        }

        void move_point(std::pair<double, double> orient, T dest) {
            double dist = orient.first * orient.first + orient.second * orient.second;
            x += dest * orient.first / dist;
            y += dest * orient.first / dist;
        }

        Point<T> create_point(double direction, T dest) {
            double x_new = x + dest * cos(direction);
            double y_new = y + dest * sin(direction);
            return {x_new, y_new};
        }

        Point<T> create_point(std::pair<double, double> orient, T dest) {
            double dist = orient.first * orient.first + orient.second * orient.second;
            double x_new = x + dest * orient.first / dist;
            double y_new = y + dest * orient.second / dist;
            return {x_new, y_new};
        }
    };

    struct LineSegment {
        Point<> leftEdge_, rightEdge_;
        double slope_, intercept_, orient_;
        static int num_lines;

        // delete default constructor
        LineSegment() = delete;

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

        bool is_intersect(Point<> &point1, Point<> &point2) const;

        Point<> find_intersection(Point<> &point1, Point<> &point2) const;
    };


    struct BaselineSeg : public LineSegment {
        using TransectBasePoint = Point<>;
        double spacing_, offset_, spacing_leftover_;
        std::vector<TransectBasePoint> transects_base_points_;

        // constructor
        BaselineSeg(double spacing, double offset, const Point<> &leftEdge,
                    const Point<> &rightEdge, double spacing_leftover);

        // element accessing
        const TransectBasePoint &operator[](const unsigned int n) const {
            return transects_base_points_[n];
        }

    };

    struct TransectLine : public LineSegment {
        Point<> transect_base_point_;
        double transect_length_, baseline_orient_, transect_orient_;
        int transect_id_;

        TransectLine(Point<> &transect_base, double transect_length, double baseline_orient, int transect_id)
                : LineSegment(create_transect(transect_base, baseline_orient, transect_length)),
                  transect_base_point_(transect_base), transect_orient_(baseline_orient + PI / 2),
                  transect_length_(transect_length), baseline_orient_(baseline_orient),
                  transect_id_(transect_id) {}

        static LineSegment create_transect(Point<> &transect_base, double baseline_orient, double transect_length);
    };

    struct Baseline : public MultiLine<Point<double>> {
        using BaselinesVertex = Point<double>;

        double transect_length_;
        double spacing_;
        double offset_;
        int baseline_id_;
        std::vector<Point<double>> transects_base_pints_;
        std::vector<TransectLine> transects_lines_;

        Baseline(const std::vector<BaselinesVertex> &points, double transect_length,
                 double spacing, int baseline_id, double offset, int smooth_factor);

        [[nodiscard]] const size_t size() const override {
            return transects_base_pints_.size();
        };

        [[nodiscard]] const BaselinesVertex &operator[](size_t i) const override {
            return transects_base_pints_[i];
        }

    };
}


#endif //DSAS_CPP_GEOMETRY_H
