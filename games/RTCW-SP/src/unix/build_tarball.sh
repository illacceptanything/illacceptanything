#!/bin/sh
# temporary script to build a tarball out of a build tree

IN_DIR=$1
VERSION=$2

TMP_DIR=wolf-$VERSION
rm -rf $TMP_DIR
mkdir $TMP_DIR

echo "Building tarball from $IN_DIR"
echo "Version $VERSION"

cp -R $IN_DIR/* $TMP_DIR
find $TMP_DIR -name '*.so' -exec strip {} \;
strip $TMP_DIR/wolf.x86

rm wolf-linux-$VERSION.tar.gz
tar cvzf wolf-linux-$VERSION.tar.gz $TMP_DIR

echo "done: wolf-linux-$VERSION.tar.gz"
