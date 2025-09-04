FROM auchida/freebsd:latest

LABEL maintainer="钟智强 <johnmelodymel@qq.com>"

RUN pkg update -y && pkg upgrade -y && \
    pkg install -y git cmake gmake curl libmicrohttpd libcjson bash && \
    pkg clean -y

WORKDIR /app

COPY . /app

RUN mkdir -p build && cd build && cmake .. && gmake

EXPOSE 8080

CMD ["./build/mo"]
