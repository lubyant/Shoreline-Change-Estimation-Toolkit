import os
import sys
import subprocess
from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions.")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(
            self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        # Setup environment variables for Conda
        env = os.environ.copy()
        env['CMAKE_PREFIX_PATH'] = os.environ.get(
            'CONDA_PREFIX', "")  # Ensures CMake uses Conda's path

        cfg = 'Debug' if self.debug else 'Release'
        cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        subprocess.check_call(['cmake', ext.sourcedir] +
                              cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(
            ['cmake', '--build', '.', '--target', ext.name], cwd=self.build_temp)


setup(
    name='SCET',
    version='0.1',
    author='Your Name',
    description='A C++ extension for Python',
    long_description='',
    ext_modules=[CMakeExtension('cppext')],
    packages=find_packages(),
    cmdclass={'build_ext': CMakeBuild},
    zip_safe=False,
)
