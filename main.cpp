#include <iostream>
#include "geometry.h"
#define PI 3.1415926

int main() {
    using namespace gm;
    Point point1(0, 0);
    std::cout << point1 << "\n";
    Point point2 = point1.createPoint(45.0/180.0*PI, 1);
    std::cout << point2 << "\n";

    std::cout << point1.distanceToPoint(point2) << "\n";

    LineSegment line1(point1, point2);

    std::cout << line1.slope << "," << line1.intercept << "\n";

    LineSegment line2 = line1.createLine(1);

    std::cout << line1 << "\n" << line2 << "\n";

    return 0;
}
