#!/bin/sh

MEDIADIR=$HOME/Id/media/WolfMedia-GOTY-maps
MAKESELF=makeself/makeself.sh
SPDIR=../../../WolfSP

VERSION=`cat $SPDIR/src/game/q_shared.h | grep Q3_VERSION | sed -e 's/.*Wolf \(.*\)"/\1/'`
echo "Building GOTY map pack (version $VERSION)"
DIRNAME=GOTY-map-$VERSION

rm -rf $DIRNAME
mkdir $DIRNAME

# we rely on new sdk-setup, and push our own tweaks on top
cp -R sdk-setup/* $DIRNAME
cp -R mappack-setup/* $DIRNAME

# copy the content
cp -R $MEDIADIR/* $DIRNAME

# final pass: cleanup CVS entries
find $DIRNAME -name CVS -exec rm -rf {} \; 2>/dev/null

# build the setup --------------------
$MAKESELF $DIRNAME wolf-linux-GOTY-maps.x86.run "Return To Castle Wolfenstein GOTY Map Pack" ./setup.sh $VERSION
