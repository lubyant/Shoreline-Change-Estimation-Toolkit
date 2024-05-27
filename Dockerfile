# Stage 1: Build dependencies
FROM ubuntu:latest as builder

LABEL authors="lubyant1994"

# Install all required dependencies
RUN apt update && apt install -y \
    build-essential \
    cmake \
    libopencv-dev \
    libgdal-dev \
    libboost-all-dev \
    libgtest-dev

# Copy the source code into the image
COPY . /app

# Build the project
WORKDIR /app/build
RUN cmake ..
RUN make

# Stage 2: Prepare the final image
FROM ubuntu:latest

# Copy the built artifacts from the builder stage
COPY --from=builder /app/build /app/build

# Set working directory
WORKDIR /app/build

# Set the command to run tests when the container starts
CMD ["ctest"]

# Set the ENTRYPOINT to keep the container running
ENTRYPOINT ["top", "-b"]