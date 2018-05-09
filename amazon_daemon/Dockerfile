FROM ubuntu:16.04

RUN apt-get update \
&& apt-get install -y \
autoconf \
automake \
libtool \
curl \
make \
g++ \
unzip \
&& apt-get clean

ADD ./protobuf-cpp-3.5.1.zip /protobuf-cpp-3.5.1.zip
RUN unzip protobuf-cpp-3.5.1.zip
WORKDIR /protobuf-3.5.1


RUN ./configure --prefix=/usr
RUN make
RUN make check
RUN make install
RUN ldconfig


RUN mkdir /code
WORKDIR /code

RUN mkdir /config
ADD ./config /config/

RUN apt-get install -y \
libpqxx-dev \
valgrind \
&& apt-get clean

ADD ./code /code/

RUN chmod +x /config/entrypoint.sh
RUN chmod +rx main.cpp
