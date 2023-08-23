FROM alpine
USER root
RUN apk update && \
    apk add \
      gcc make musl-dev bash \
      sdl2-dev sdl2_image-dev

RUN addgroup -S schwein && adduser -S schwein -G schwein
USER schwein
WORKDIR /home/schwein

