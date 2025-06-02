FROM quay.io/pypa/manylinux_2_28_x86_64

# Install essential build tools
RUN yum install -y epel-release && \
    yum install -y \
    gcc gcc-c++ make cmake ninja-build \
    libgomp \
    gdal-devel opencv-devel boost-devel \
    python3-devel

# Use Python 3.10 explicitly
RUN /opt/python/cp310-cp310/bin/pip install --upgrade \
    setuptools wheel build scikit-build-core[pyproject] pybind11[global]

# Set paths explicitly for Python 3.10
ENV PATH="/opt/python/cp310-cp310/bin:$PATH"
ENV CMAKE_PREFIX_PATH="/usr"

WORKDIR /io

# Build wheel (assumes pyproject.toml is present)
# You’ll run this at build time:
# docker build -t scet .
# docker run --rm -v $(pwd):/io scet bash -c "python3 -m build && auditwheel repair dist/*.whl -w dist/"

