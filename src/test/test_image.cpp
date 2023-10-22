//
// Created by lby on 10/14/23.
//

#include "gtest/gtest.h"
#include "../dsas/image.h"
#include <filesystem>
#include "../dsas/utility.h"

TEST(TestImage, TestLoad) {
    using namespace std;
    filesystem::path image_path{"/home/lby/Desktop/DSAS_cpp/img/final_raster/4108603/4108603_2018.tif"};
    auto *img = new dsas::Image(image_path);
    EXPECT_EQ(img->shorelines_.size(), 1);
    delete img;
}
