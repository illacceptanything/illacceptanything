#!/bin/sh
# put together everything for mod sdk
# no setup, just a zip
# this is both MP and SP
# we use the version from the SP tree

MEDIADIR=$HOME/Id/media/WolfMedia-sdk
MAKESELF=makeself/makeself.sh
SPDIR=../../../WolfSP
MPDIR=../..
VERSION=`cat $SPDIR/src/game/q_shared.h | grep Q3_VERSION | sed -e 's/.*Wolf \(.*\)"/\1/'`
echo "Building mod sdk for version $VERSION"
DIRNAME=RTCW-mod-sdk-$VERSION
TESTDIR=RTCW-mod-sdk-test

rm -rf $DIRNAME

# SP ---------------

mkdir -p $DIRNAME/SP/src/unix
mkdir -p $DIRNAME/SP/main/ui

cp -R $SPDIR/src/botai $DIRNAME/SP/src
cp -R $SPDIR/src/cgame $DIRNAME/SP/src
cp -R $SPDIR/src/game $DIRNAME/SP/src
cp -R $SPDIR/src/ui $DIRNAME/SP/src

cp -R $SPDIR/main/ui/menudef.h $DIRNAME/SP/main/ui

# the build system
cp $SPDIR/src/unix/Construct $DIRNAME/SP/src/unix
cp $SPDIR/src/unix/Conscript-game $DIRNAME/SP/src/unix
cp $SPDIR/src/unix/Conscript-cgame $DIRNAME/SP/src/unix
cp $SPDIR/src/unix/Conscript-ui $DIRNAME/SP/src/unix
cp $SPDIR/src/unix/cons $DIRNAME/SP/src/unix

# extractfuncs binary
(
cd $SPDIR/src/unix
./cons extractfuncs
strip extractfuncs
)
cp $SPDIR/src/unix/extractfuncs $DIRNAME/SP/src/unix

# MP ----------------

mkdir -p $DIRNAME/MP/src/unix
mkdir -p $DIRNAME/MP/MAIN/ui_mp

cp -R $MPDIR/src/botai $DIRNAME/MP/src
cp -R $MPDIR/src/cgame $DIRNAME/MP/src
cp -R $MPDIR/src/game $DIRNAME/MP/src
cp -R $MPDIR/src/ui $DIRNAME/MP/src

cp -R $MPDIR/MAIN/ui_mp/menudef.h $DIRNAME/MP/MAIN/ui_mp

# the build system
cp $MPDIR/src/unix/Construct $DIRNAME/MP/src/unix
cp $MPDIR/src/unix/Conscript-game $DIRNAME/MP/src/unix
cp $MPDIR/src/unix/Conscript-cgame $DIRNAME/MP/src/unix
cp $MPDIR/src/unix/Conscript-ui $DIRNAME/MP/src/unix
cp $MPDIR/src/unix/cons $DIRNAME/MP/src/unix
cp $MPDIR/src/unix/ldd_check.pm $DIRNAME/MP/src/unix

# static content (MP & SP) -----------
cp -R $MEDIADIR/* $DIRNAME

# copy setup dirs --------------------
cp -R sdk-setup/* $DIRNAME

# final pass: cleanup CVS entries
find $DIRNAME -name CVS -exec rm -rf {} \; 2>/dev/null

# proceed to testing this ------------
if [ `hostname` == vmspoutnik32 ]
then
  rm -rf $TESTDIR
  cp -R $DIRNAME $TESTDIR
  cd $TESTDIR  
    cd MP/src/unix
    ./cons || exit
    cd ../../..
    cd SP/src/unix
    ./cons || exit
    cd ../../..
  cd ..
fi

# build the setup --------------------
$MAKESELF $DIRNAME wolf-linux-sdk-$VERSION.x86.run "Return To Castle Wolfenstein MOD SDK" ./setup.sh $VERSION

exit 0
