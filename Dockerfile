FROM quay.io/pypa/manylinux_2_28_x86_64

ENV CONDA_DIR=/opt/conda
ENV PATH="$CONDA_DIR/bin:$PATH"

# Install Miniconda
RUN curl -sLo miniconda.sh https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh && \
    bash miniconda.sh -b -p $CONDA_DIR && \
    rm miniconda.sh

RUN $CONDA_DIR/bin/conda create -y -n buildenv python=3.10 && \
    $CONDA_DIR/bin/conda run -n buildenv conda install -y -c conda-forge \
        pybind11 cmake make ninja \
        gdal opencv boost \
        pillow rasterio pyproj tqdm ftfy regex rhash && \
    $CONDA_DIR/bin/conda run -n buildenv conda install -y -c pytorch \
        pytorch==1.12.1 torchvision==0.13.1 torchaudio==0.12.1 cudatoolkit=11.3 && \
    $CONDA_DIR/bin/conda run -n buildenv pip install -U openmim build && \
    $CONDA_DIR/bin/conda run -n buildenv mim install mmengine && \
    $CONDA_DIR/bin/conda run -n buildenv mim install "mmcv==2.0.0rc4" && \
    $CONDA_DIR/bin/conda run -n buildenv pip install "mmsegmentation>=1.0.0"


# Set environment for buildenv
ENV PATH="$CONDA_DIR/envs/buildenv/bin:$PATH"
ENV LD_LIBRARY_PATH="$CONDA_DIR/envs/buildenv/lib:$LD_LIBRARY_PATH"

# Set working directory
WORKDIR /io

# If needed, copy your project (or use docker mount with -v)
# COPY . /io

# Build wheel
# RUN python -m build

# Repair with auditwheel
# RUN auditwheel repair dist/*.whl -w dist/
