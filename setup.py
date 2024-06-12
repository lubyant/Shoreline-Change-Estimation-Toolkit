from setuptools import setup, Extension
import pybind11

setup(
    name="dasa",
    version="0.1",
    ext_modules=[
        Extension("dsas",
                  ["shorelinecalculator/geometry.cpp",
                   "shorelinecalculator/image.cpp",
                   "shorelinecalculator/shorelinecalculator.cpp",
                   "shorelinecalculator/utility.cpp",],
                  include_dirs=[pybind11.get_include()],
                  extra_compile_args=["-std=c++17"])
    ]
    setup_requires=['pybind11>=2.5.0'],
    zip_safe=False,
)
from glob import glob
from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension

ext_modules = [
    Pybind11Extension(
        "python_example",
        sorted(glob("shorelinecalculator/*.cpp")), 
    ),
]

setup(..., ext_modules=ext_modules)