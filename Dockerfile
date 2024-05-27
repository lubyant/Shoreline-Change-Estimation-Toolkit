FROM ubuntu:latest
LABEL authors="lby"

WORKDIR /app

RUN apt update
RUN apt install -y build-essential cmake libopencv-dev gdal-bin libboost-all-dev libgtest-dev

COPY . /app/ShorelineCalculator

RUN mkdir build && cmake --build build

ENTRYPOINT ["top", "-b"]