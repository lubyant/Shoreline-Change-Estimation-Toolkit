FROM quay.io/pypa/manylinux_2_28_x86_64

# ---------- Versions ----------
ENV SQLITE_VERSION=3450300
ENV GEOS_VERSION=3.12.1
ENV GDAL_VERSION=3.11.0

# ---------- Install system dependencies ----------
RUN yum install -y epel-release && \
    yum install -y \
    gcc gcc-c++ make cmake3 ninja-build \
    libgomp wget tar unzip swig \
    python3 python3-devel python3-pip \
    libtiff-devel libjpeg-devel libpng-devel zlib-devel \
    libcurl-devel expat-devel proj-devel \
    libxml2-devel libwebp-devel \
    opencv opencv-devel \
    boost boost-devel libstdc++-static

# ---------- Set up Python environment for manylinux Python ----------
ENV PATH="/opt/python/cp310-cp310/bin:$PATH"
ENV CMAKE_PREFIX_PATH="/usr/local"
ENV LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib64:$LD_LIBRARY_PATH"
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++

RUN /opt/python/cp310-cp310/bin/pip install --upgrade pip setuptools wheel numpy

# ---------- Build and install SQLite with RTree ----------
WORKDIR /tmp
RUN wget https://www.sqlite.org/2024/sqlite-autoconf-${SQLITE_VERSION}.tar.gz && \
    tar -xzf sqlite-autoconf-${SQLITE_VERSION}.tar.gz && \
    cd sqlite-autoconf-${SQLITE_VERSION} && \
    ./configure --prefix=/usr/local --enable-rtree && \
    make -j$(nproc) && make install && \
    cd .. && rm -rf sqlite-autoconf-${SQLITE_VERSION}*

# ---------- Build and install GEOS ----------
WORKDIR /tmp
RUN wget https://download.osgeo.org/geos/geos-${GEOS_VERSION}.tar.bz2 && \
    tar -xjf geos-${GEOS_VERSION}.tar.bz2 && \
    cd geos-${GEOS_VERSION} && \
    cmake3 -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake3 --build build --parallel && \
    cmake3 --install build && \
    cd .. && rm -rf geos-${GEOS_VERSION}*

# ---------- Build and install GDAL from source ----------
RUN wget https://download.osgeo.org/gdal/${GDAL_VERSION}/gdal-${GDAL_VERSION}.tar.gz && \
    tar -xzf gdal-${GDAL_VERSION}.tar.gz

WORKDIR /tmp/gdal-${GDAL_VERSION}
RUN mkdir build && cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DBUILD_SHARED_LIBS=ON \
      -DGDAL_BUILD_OPTIONAL_DRIVERS=ON \
      -DSQLITE3_INCLUDE_DIR=/usr/local/include \
      -DSQLITE3_LIBRARY=/usr/local/lib/libsqlite3.so \
      -DGEOS_INCLUDE_DIR=/usr/local/include \
      -DGEOS_LIBRARY=/usr/local/lib/libgeos_c.so \
      -DGDAL_USE_GEOS=ON \
      -DSWIG_EXECUTABLE=/usr/bin/swig \
      -DBUILD_PYTHON_BINDINGS=OFF \
      -DWITH_WEBP=ON \
      -DWITH_PNG=ON \
      -DWITH_JPEG=ON \
      -DWITH_EXPAT=ON \
      -DWITH_CURL=ON \
      -DWITH_LIBTIFF=ON && \
    cmake --build . -- -j$(nproc --ignore=2) && \
    cmake --install . && \
    echo "/usr/local/lib" > /etc/ld.so.conf.d/gdal.conf && ldconfig && \
    cd /tmp && rm -rf gdal-${GDAL_VERSION}

# ---------- Final Python tools for wheel build ----------
RUN pip install --upgrade \
    build scikit-build-core[pyproject] pybind11[global]

# ---------- Build target ----------
WORKDIR /io
CMD ["bash", "-c", "\
    which python3 && python3 --version && \
    python3 -m build && \
    auditwheel repair dist/*.whl -w dist/"]
