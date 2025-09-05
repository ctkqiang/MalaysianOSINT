FROM ubuntu:22.04

LABEL maintainer="钟智强 <johnmelodymel@qq.com>"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    git \
    cmake \
    make \
    g++ \
    curl \
    pkg-config \
    libmicrohttpd-dev \
    libcjson-dev \
    libcurl4-openssl-dev \
    bash && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app

RUN mkdir -p build && cd build && cmake .. && make -j$(nproc)

EXPOSE 8080

CMD ["./build/mo"]
