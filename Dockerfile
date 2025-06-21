FROM quay.io/pypa/manylinux_2_28_x86_64

# Set environment variables
ENV SQLITE_VERSION=3450300
ENV PROJ_VERSION=9.3.1
ENV GDAL_VERSION=3.8.5
ENV BOOST_VERSION=1.83.0
ENV BOOST_VERSION_UNDERSCORE=1_83_0
ENV OPENCV_VERSION=4.9.0

# Step 0: Install base system dependencies
RUN yum install -y epel-release && \
    yum install -y \
    gcc gcc-c++ make cmake3 ninja-build \
    zlib-devel expat-devel \
    libtiff-devel libcurl-devel \
    python3-devel wget tar \
    bzip2-devel unzip git libgomp 

# Step 1: Build and install SQLite3 from source (with RTree)
RUN wget https://www.sqlite.org/2024/sqlite-autoconf-${SQLITE_VERSION}.tar.gz && \
    tar -xzf sqlite-autoconf-${SQLITE_VERSION}.tar.gz && \
    cd sqlite-autoconf-${SQLITE_VERSION} && \
    ./configure --prefix=/usr/local --enable-rtree && \
    make -j$(nproc) && make install && \
    cd .. && rm -rf sqlite-autoconf-${SQLITE_VERSION}*

# Step 2: Build and install PROJ (static)
RUN wget https://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz && \
    tar -xzf proj-${PROJ_VERSION}.tar.gz && \
    cd proj-${PROJ_VERSION} && \
    mkdir build && cd build && \
    cmake3 .. -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX=/usr/local \
              -DBUILD_SHARED_LIBS=OFF \
              -DBUILD_TESTING=OFF && \
    make -j$(nproc) && make install && \
    cd ../.. && rm -rf proj-${PROJ_VERSION}*

# Step 3: Build and install GDAL (static)
RUN wget https://download.osgeo.org/gdal/${GDAL_VERSION}/gdal-${GDAL_VERSION}.tar.gz && \
    tar -xzf gdal-${GDAL_VERSION}.tar.gz && \
    cd gdal-${GDAL_VERSION} && \
    mkdir build && cd build && \
    cmake3 .. -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX=/usr/local \
              -DBUILD_SHARED_LIBS=OFF \
              -DGDAL_USE_TIFF=ON \
              -DGDAL_USE_LIBICONV_INTERNAL=ON && \
    make -j$(nproc) && make install && \
    cd ../.. && rm -rf gdal-${GDAL_VERSION}*

# Step 4: Build and install Boost (static with date_time)
RUN wget https://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/boost_${BOOST_VERSION_UNDERSCORE}.tar.gz/download -O boost_${BOOST_VERSION_UNDERSCORE}.tar.gz && \
    tar -xzf boost_${BOOST_VERSION_UNDERSCORE}.tar.gz && \
    cd boost_${BOOST_VERSION_UNDERSCORE} && \
    ./bootstrap.sh --with-libraries=date_time && \
    ./b2 link=static install --prefix=/usr/local && \
    cd .. && rm -rf boost_${BOOST_VERSION_UNDERSCORE}*


# Step 5: Build and install OpenCV (minimal static)
RUN git clone --branch ${OPENCV_VERSION} https://github.com/opencv/opencv.git && \
    mkdir -p opencv/build && cd opencv/build && \
    cmake3 .. -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX=/usr/local \
              -DBUILD_SHARED_LIBS=OFF \
              -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF \
              -DBUILD_EXAMPLES=OFF -DBUILD_opencv_python_bindings_generator=OFF \
              -DBUILD_opencv_python3=OFF && \
    make -j$(nproc) && make install && \
    cd ../.. && rm -rf opencv

# Step 6: Python build environment
ENV PATH="/opt/python/cp310-cp310/bin:$PATH"
ENV CMAKE_PREFIX_PATH="/usr/local"
ENV LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++

# Step 7: Install Python build tools
RUN pip install --upgrade \
    setuptools wheel build scikit-build-core[pyproject] pybind11[global]

# Step 8: Set working directory
WORKDIR /io

# Step 9: Build and repair the wheel
CMD ["bash", "-c", "\
    python3 -m build && \
    auditwheel repair dist/*.whl -w dist/"]
