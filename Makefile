NAME=mrkrstphr/illacceptanything
REF=HEAD

all: prepare build

prepare:
	mkdir -p docker
	git archive -o docker/spaceman-git-archive.tar $(REF)

build:
	docker build -t $(NAME):latest --rm .
