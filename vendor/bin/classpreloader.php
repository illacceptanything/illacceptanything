#!/usr/bin/env sh
SRC_DIR="`pwd`"
cd "`dirname "$0"`"
cd "../classpreloader/classpreloader"
BIN_TARGET="`pwd`/classpreloader.php"
cd "$SRC_DIR"
"$BIN_TARGET" "$@"
