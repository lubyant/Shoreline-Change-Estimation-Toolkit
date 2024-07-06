#include <pybind11/pybind11.h>

#include <iostream>
namespace py = pybind11;
#include "shorelinecalculator.hpp"

void generate_result_from_folder(const std::string &input_folder,
                                 const std::string &output_folder,
                                 const dsas::Options &options) {
  dsas::digital_shoreline_analysis_system(input_folder, output_folder, options);
}

void generate_result_from_image(const std::string &image_path,
                                const std::string &output_folder,
                                const dsas::Options &options) {
  auto img = dsas::Image(image_path, options);
  auto psz_prj_ = img.psz_prj_.c_str();
  std::vector<dsas::Image> images;
  images.push_back(std::move(img));
  auto shorelines = dsas::Image::merge_shorelines_from_images(images);
  // save the shoreline to shp
  util::save_lines<gm::Shoreline>(shorelines, psz_prj_,
                                  output_folder + "/shoreline.shp");
}

PYBIND11_MODULE(cppext, m) {
  m.doc() = "cpp extension for calculate the shoreline erosion\n";
  m.def("generate_result_from_folder", &generate_result_from_folder,
        "function take string of input folder and outputfoler\n");
  m.def("generate_result_from_image", &generate_result_from_image,
        "function take string of input image path and outputfoler\n");

  py::enum_<gm::IntersectionMode>(m, "IntersectionMode")
      .value("Closest", gm::IntersectionMode::Closest)
      .value("Farthest", gm::IntersectionMode::Farthest)
      .export_values();

  py::enum_<gm::TransectOrientation>(m, "TransectOrientation")
      .value("Mix", gm::TransectOrientation::Mix)
      .value("Left", gm::TransectOrientation::Left)
      .value("Right", gm::TransectOrientation::Right)
      .export_values();
  py::class_<dsas::Options>(m, "Options")
      .def(py::init<>())  // Default constructor
      .def_readwrite("smooth_factor", &dsas::Options::smooth_factor)
      .def_readwrite("edge_distance", &dsas::Options::edge_distance)
      .def_readwrite("shoreline_least_factor",
                     &dsas::Options::shoreline_least_factor)
      .def_readwrite("transect_length", &dsas::Options::transect_length)
      .def_readwrite("transect_spacing", &dsas::Options::transect_spacing)
      .def_readwrite("transect_offset", &dsas::Options::transect_offset)
      .def_readwrite("outlier_rate", &dsas::Options::outlier_rate)
      .def_readwrite("thread_num", &dsas::Options::thread_num)
      .def_readwrite("intersection_mode", &dsas::Options::intersection_mode)
      .def_readwrite("transect_orient", &dsas::Options::transect_orient);
}
