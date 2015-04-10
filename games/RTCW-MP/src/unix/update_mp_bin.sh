#!/bin/sh
# hardcoding some stuff to make it easier
# $1 : path of the existing pk3
# $2 : the path to the .so to update

UPTMP="/tmp/update_mp_bin"

CGAME="cgame.mp.i386.so"
UI="ui.mp.i386.so"
ZIPFILE="$1"
# we need absolute path
TEST=`echo "$ZIPFILE" | grep '^/'`
if [ "x" == "x$TEST" ]
then
ZIPFILE="$PWD/$ZIPFILE"
fi
shift
rm -rf $UPTMP
mkdir $UPTMP
cp $1/$CGAME $1/$UI $UPTMP
cd $UPTMP
strip *.so
zip $ZIPFILE $CGAME $UI
