#include <pybind11/pybind11.h>

#include <boost/json.hpp>
#include <iostream>
namespace py = pybind11;
#include "shorelinecalculator.hpp"
/*
{
    "input_data":{
        "image_name1":{
        "2000/1/1": "path_to_2000",
        "2005/1/1": "path_to_2005"
        },
        "image_name2":{
        "2000/1/1": "path_to_2000",
        "2005/1/1": "path_to_2005"
        }
    }
    "output_data": "output_folder",
    "options":{
        "smooth_factor": 5,
        "edge_distance": 50,
        "shoreline_least_factor": 0.4,
        "transect_length": 10.0,
        "transect_spacing": 10.0,
        "transect_offset": 10.0,
        "outlier_rate": 10.0,
        "thread_num": 100,
        "intersection_mode": "farthest",
        "transect_orientation": "left"
    }
}
*/
void cppext(const std::string &json_str);
void cppext(const std::string &json_str) {

}

PYBIND11_MODULE(cppext, m) {}