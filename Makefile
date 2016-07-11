# set shell to bash
SHELL = bash

# figure out absolute path of source repo
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# gather uname info
UNAME := $(shell uname)

ALL:
	echo -e "$(UNAME) is terrible.  Not building on this platform."
