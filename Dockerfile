FROM quay.io/pypa/manylinux_2_28_x86_64

# Step 0: Install system dependencies (do NOT use devtoolsets)
RUN yum install -y epel-release && \
    yum install -y \
    gcc gcc-c++ make cmake3 ninja-build \
    libgomp gdal-devel \
    opencv-devel boost-devel \
    wget tar python3-devel \
    libtiff-devel libjpeg-devel \
    libcurl-devel expat-devel \
    sqlite-devel proj-devel \
    geos-devel


# Step 2: Set environment variables
ENV PATH="/opt/python/cp310-cp310/bin:$PATH"
ENV CMAKE_PREFIX_PATH="/usr"
ENV LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++

# Step 3: Install Python build tools
RUN pip install --upgrade \
    setuptools wheel build scikit-build-core[pyproject] pybind11[global]

# Step 4: Set working directory
WORKDIR /io

# Step 5: Build and repair the wheel
CMD ["bash", "-c", "\
  which python3 && python3 --version && \
  python3 -m build && \
  auditwheel repair dist/*.whl -w dist/"]