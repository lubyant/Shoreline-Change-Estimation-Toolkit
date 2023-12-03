//
// Created by lby on 10/14/23.
//

#include <filesystem>

#include "../dsas/image.h"
#include "gtest/gtest.h"

TEST(TestImage, TestLoad) {
  using namespace std;
  filesystem::path image_path{
      "/home/lby/Desktop/shorecalculator/img/final_raster/4108603/"
      "4108603_2018.tif"};
  auto *img = new dsas::Image(image_path);
  EXPECT_EQ(img->year_, 2018);
  delete img;
}
