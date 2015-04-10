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

<<<<<<< HEAD:Dockerfile
    # Remove build-time dependencies
    && apt-get purge -y --auto-remove $BUILD_DEPS \
    && rm -rf /var/lib/apt/lists/*
=======
COPY . /opt/illacceptanything
>>>>>>> ff99ab7... SORT THE REPOSITORY ALREADY (Closes #490):code/Dockerfile

USER root
WORKDIR /opt/illacceptanything
ENTRYPOINT ["/bin/bash"]
