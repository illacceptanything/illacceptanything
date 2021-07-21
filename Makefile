# set shell to bash
SHELL = bash

# figure out absolute path of source repo
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# gather uname info
UNAME := $(shell uname)

ALL:
	echo -e "$(UNAME) is terrible.  Not building on this platform."
	number=1; while [[ $$number -le 256 ]]; do git commit --allow-empty --allow-empty-message -m ""; ((number = number + 1)); done
