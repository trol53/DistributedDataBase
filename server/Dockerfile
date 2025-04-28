FROM debian:latest

RUN apt update -y && apt install -y \
    build-essential autotools-dev   \
    libboost-all-dev cmake

WORKDIR /home

COPY Makefile /home
COPY server.cc /home
COPY db_interface.hpp /home
COPY lib /home/lib
COPY CMakeLists.txt /home

ENTRYPOINT [ "make" ]