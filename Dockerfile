FROM ubuntu:14.04

RUN apt-get -qq update && \
    apt-get install -yqq cowsay

ENTRYPOINT ["/usr/games/cowsay"]
CMD ["-h"]
