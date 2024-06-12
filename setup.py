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
