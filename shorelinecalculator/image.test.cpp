#include "image.hpp"

#include <boost/test/unit_test.hpp>

using namespace dsas;

BOOST_AUTO_TEST_SUITE(ImageTest)

BOOST_AUTO_TEST_CASE(test_overlaid) {
  {
    Image image1, image2;
    image1.bottom_left_ = gm::Point<double>(0, 0);
    image1.bottom_right_ = gm::Point<double>(1, 0);
    image1.up_left_ = gm::Point<double>(0, 1);
    image1.up_right_ = gm::Point<double>(1, 1);

    image2.bottom_left_ = gm::Point<double>(0.5, 0);
    image2.bottom_right_ = gm::Point<double>(1, 0);
    image2.up_left_ = gm::Point<double>(0.5, 1);
    image2.up_right_ = gm::Point<double>(1, 1);

    BOOST_CHECK(image1.is_overlaid(image2) == true);
  }
  {
    Image image1, image2;
    image1.bottom_left_ = gm::Point<double>(0, 0);
    image1.bottom_right_ = gm::Point<double>(1, 0);
    image1.up_left_ = gm::Point<double>(0, 1);
    image1.up_right_ = gm::Point<double>(1, 1);

    image2.bottom_left_ = gm::Point<double>(1.5, 0);
    image2.bottom_right_ = gm::Point<double>(2, 0);
    image2.up_left_ = gm::Point<double>(1.5, 1);
    image2.up_right_ = gm::Point<double>(2, 1);

    BOOST_CHECK(image1.is_overlaid(image2) == false);
  }
}
BOOST_AUTO_TEST_SUITE_END()