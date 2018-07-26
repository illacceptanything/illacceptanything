FROM debian:7.8

ENV \
    RUN_DEPS="python" \
    BUILD_DEPS="python-pip" \
    DEBIAN_FRONTEND="noninteractive"

ADD . /opt/illacceptanything

RUN \
    # Install dependencies
    apt-get update \
    && apt-get -y install $BUILD_DEPS $RUN_DEPS \

    # Do stuff... but what?

COPY . /opt/illacceptanything

USER root
WORKDIR /opt/illacceptanything
ENTRYPOINT ["/bin/bash"]
