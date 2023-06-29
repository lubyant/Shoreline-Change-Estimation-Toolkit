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
        Point leftEdge_, rightEdge_;
        double slope_, intercept_, orient_;
        static int num_lines;

        // constructor
        LineSegment();

        LineSegment(Point leftEdge, Point rightEdge);

        // copy constructor
        LineSegment(const LineSegment &lineSegment);

        // move constructor
        LineSegment(LineSegment &&lineSegment) noexcept;

        // copy assignment
        LineSegment &operator=(const LineSegment &lineSegment);

        // move assignment
        LineSegment &operator=(LineSegment &&lineSegment) noexcept;

        // destructor
        virtual ~LineSegment() { num_lines--; }

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
        std::vector<std::unique_ptr<Point>> *transects_ptr_;

        // constructor
        BaselineSeg(double spacing, double offset, const Point &leftEdge, const Point &rightEdge,
                    double spacing_leftover);

        // default constructor
        BaselineSeg()
                : LineSegment(), spacing_leftover_(0), offset_(0), spacing_(0),
                  transects_ptr_(new std::vector<std::unique_ptr<Point>>()) {};

        // copy constructor
        BaselineSeg(const BaselineSeg &baselineSeg);

        // copy operator
        BaselineSeg &operator=(const BaselineSeg &baselineSeg);

        // move constructor
        BaselineSeg(BaselineSeg &&baselineSeg) noexcept;

        // move operator
        BaselineSeg &operator=(BaselineSeg &&baselineSeq) noexcept;

        // destructor
        ~BaselineSeg() override { delete transects_ptr_; }

        // element accessing
        const Point &operator[](const unsigned int n) const { return *transects_ptr_->at(n); }

        // element modifying
        Point &operator[](const unsigned int n) { return *transects_ptr_->at(n); }
    };


    class Shorelines {
    public:
        std::string year;

        unsigned int id;

        std::vector<std::unique_ptr<Point>> *shore_ptr_;

        std::vector<std::unique_ptr<LineSegment>> *shores_;

        [[nodiscard]] unsigned long size() const { return shore_ptr_->size(); }

        // default constructor
        Shorelines();

        // constructor
        Shorelines(const std::vector<Point> &shore_points, std::string year, unsigned int shoreline_id = 0);

        // copy constructor
        Shorelines(const Shorelines &shorelines);

        // copy operator
        Shorelines &operator=(const Shorelines &shorelines);

        // move constructor
        Shorelines(Shorelines &&shorelines) noexcept;

        // move operator
        Shorelines &operator=(Shorelines &&shorelines) noexcept;

        // destructor
        ~Shorelines() {
            delete shore_ptr_;
            delete shores_;
        };

        // element accessing
        const Point &operator[](unsigned int i) const { return *shore_ptr_->at(i); }

        // element modifying
        Point &operator[](unsigned int i) { return *shore_ptr_->at(i); }

        void pushBack(double x, double y) { shore_ptr_->push_back(std::make_unique<Point>(x, y)); }

        void pushFront(double x, double y) { shore_ptr_->insert(shore_ptr_->begin(), std::make_unique<Point>(x, y)); }

        [[nodiscard]] std::vector<std::unique_ptr<LineSegment>> shores() const noexcept;
    };

    class TransectLine : public LineSegment {
    public:
        Point transect_base_;

        double transect_length_, baseline_orient_, transect_orient_;

        static LineSegment create_transect(gm::Point &transect_base, double baseline_orient, double transect_length);

        // default constructor
        TransectLine()
                : LineSegment(), transect_orient_(0), transect_length_(0), baseline_orient_(0), transect_base_() {}

        // constructor
        TransectLine(Point &transect_base, double transect_length, double baseline_orient);

        // copy constructor
        TransectLine(const TransectLine &transectLine) = default;

        // copy0 operator
        TransectLine &operator=(const TransectLine &transectLine) = default;

        // move constructor
        TransectLine(TransectLine &&transectLine) = default;

        // move operator
        TransectLine &operator=(TransectLine &&transectLine) = default;

        // destructor
        ~TransectLine() override = default;

    };

    class Baselines {
    public:
        double transect_length_;
        double spacing_;
        double offset_;
        int baseline_id;
        std::vector<std::unique_ptr<Point>> *transects_; // vector to store transects base
        std::vector<std::unique_ptr<TransectLine>> *transects_line_; // vector to transects
        std::vector<std::unique_ptr<BaselineSeg>> *baselines_; // vector to baselines

        // constructor
        Baselines(std::vector<Point> &baseline_points, double transect_length, double spacing,
                  int baseline_id, double offset);

        // default constructor
        Baselines()
                : transect_length_(0), spacing_(0), offset_(0), baseline_id(0),
                  transects_(new std::vector<std::unique_ptr<Point>>()),
                  transects_line_(new std::vector<std::unique_ptr<TransectLine>>()),
                  baselines_(new std::vector<std::unique_ptr<BaselineSeg>>()) {}

        // move constructor
        Baselines(Baselines &&baselines) noexcept;

        // move operator
        Baselines &operator=(Baselines &&baselines) noexcept;

        // copy constructor
        Baselines(const Baselines &baselines);

        // copy operator
        Baselines &operator=(const Baselines &baselines);

        // destructor
        ~Baselines() {
            delete transects_;
            delete transects_line_;
            delete baselines_;
        }

        [[nodiscard]] unsigned int size() const { return baselines_->size(); }

        const BaselineSeg &operator[](unsigned int i) const { return *baselines_->at(i); }

        std::vector<std::unique_ptr<Point>> intersect_shorelines(Shorelines &shorelines) const;


    };

}


#endif //SHORECALCULATOR_GEOMETRY_H
