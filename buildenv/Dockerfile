FROM alpine
USER root
RUN apk update && \
    apk add \
      gcc make musl-dev git python3 py3-pip bash \
      sdl2-dev inih inih-dev

RUN git clone https://github.com/emscripten-core/emsdk.git && \
  cd emsdk && \
  ./emsdk install latest && \
  ./emsdk activate latest

