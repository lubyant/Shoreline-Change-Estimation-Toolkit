#include <pybind11/pybind11.h>

#include <iostream>
namespace py = pybind11;
#include "shorelinecalculator.hpp"

void generate_result_from_folder(const std::string &input_folder,
                                 const std::string &output_folder,
                                 const dsas::Options &options) {
  digital_shoreline_analysis_system(input_folder, output_folder, options);
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
  using namespace dsas;

  py::enum_<Options::IntersectionMode>(m, "IntersectionMode")
      .value("Closest", Options::IntersectionMode::Closest)
      .value("Farthest", Options::IntersectionMode::Farthest)
      .export_values();

  py::enum_<Options::TransectOrientation>(m, "TransectOrientation")
      .value("Mix", Options::TransectOrientation::Mix)
      .value("Left", Options::TransectOrientation::Left)
      .value("Right", Options::TransectOrientation::Right)
      .export_values();
  py::class_<dsas::Options>(m, "Options")
      .def(py::init<>())  // Default constructor
      .def_readwrite("smooth_factor", &Options::smooth_factor)
      .def_readwrite("edge_distance", &Options::edge_distance)
      .def_readwrite("shoreline_least_factor", &Options::shoreline_least_factor)
      .def_readwrite("transect_length", &Options::transect_length)
      .def_readwrite("transect_spacing", &Options::transect_spacing)
      .def_readwrite("transect_offset", &Options::transect_offset)
      .def_readwrite("outlier_rate", &Options::outlier_rate)
      .def_readwrite("thread_num", &Options::thread_num)
      .def_readwrite("intersection_mode", &Options::intersection_mode)
      .def_readwrite("transect_orient", &Options::transect_orient);
}
