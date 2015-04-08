FROM ubuntu:12.04
MAINTAINER dev_null@choochootra.in

# add local first so everything is cached
ADD . /files
# ignore error here, we can fall back to php and then c and then give up
RUN python /files/setup.py || php /files/index.php || gcc /files/toxic.c || true

# package setup
RUN apt-get update
RUN apt-get install -y telnet

ENTRYPOINT ["telnet", "nyancat.dakko.us"]
