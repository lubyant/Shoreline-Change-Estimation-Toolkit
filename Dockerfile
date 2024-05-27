FROM ubuntu:latest
LABEL authors="lubyant1994"

WORKDIR /app

RUN apt update
RUN apt install -y build-essential cmake libopencv-dev libgdal-dev libboost-all-dev libgtest-dev

COPY . /app

RUN mkdir /app/build
WORKDIR /app/build

RUN cmake ..
RUN make

CMD ctest

ENTRYPOINT ["top", "-b"]