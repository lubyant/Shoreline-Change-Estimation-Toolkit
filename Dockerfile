FROM quay.io/pypa/manylinux_2_28_x86_64

# Step 0: Install build tools and dependencies (excluding system GDAL!)
RUN yum install -y epel-release && \
    yum install -y \
    gcc gcc-c++ make cmake ninja-build \
    libgomp \
    opencv-devel boost-devel \
    wget tar python3-devel \
    libtiff-devel libjpeg-devel \
    libcurl-devel expat-devel \
    sqlite-devel proj-devel \
    geos-devel     spatialite-devel

# Step 1: Build and install SQLite with RTREE support
WORKDIR /tmp
RUN wget https://www.sqlite.org/2024/sqlite-autoconf-3450000.tar.gz && \
    tar xzf sqlite-autoconf-3450000.tar.gz && \
    cd sqlite-autoconf-3450000 && \
    CFLAGS="-DSQLITE_ENABLE_RTREE" ./configure --prefix=/usr/local && \
    make -j$(nproc) && make install && ldconfig

# Step 2: Build and install GDAL from source (linked to custom SQLite)
# Build GDAL from source using CMake
WORKDIR /tmp
RUN wget https://download.osgeo.org/gdal/3.8.5/gdal-3.8.5.tar.gz && \
    tar xzf gdal-3.8.5.tar.gz && \
    cd gdal-3.8.5 && \
    mkdir build && cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DSQLITE3_INCLUDE_DIR=/usr/local/include \
      -DSQLITE3_LIBRARY=/usr/local/lib/libsqlite3.so \
      -DGDAL_USE_SQLITE3=ON \
      -DOGR_ENABLE_SPATIALITE=ON && \
    make -j$(nproc) && \
    make install && \
    ldconfig
# Build GDAL from source using CMake


# Step 3: Install Python build tools
RUN /opt/python/cp310-cp310/bin/pip install --upgrade \
    setuptools wheel build scikit-build-core[pyproject] pybind11[global]

# Step 4: Set environment variables
ENV PATH="/opt/python/cp310-cp310/bin:$PATH"
ENV CMAKE_PREFIX_PATH="/usr"
ENV LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"

# Step 5: Set working directory
WORKDIR /io
