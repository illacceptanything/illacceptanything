#!/bin/sh

usage()
{
echo "syntax: build_setup.sh <setup directory> <version>"
echo "  builds an SP demo version setup"
}

check_brandelf()
{
  # make sure brandelf is installed to avoid any problem when building the setups
  BRAND=`which brandelf`;
  if [ -n "$BRAND" ] && [ -x "$BRAND" ]
  then
    echo "brandelf is present: $BRAND"
  else
    echo "brandelf not found"
    exit
  fi
}

# safe checks
check_brandelf

# process command line
if [ $# -ne 2 ]
then
  echo "bad options"
  usage
  exit
fi
WOLFBIN=$1
VERSION=$2

echo "Building setup ================================"
echo "Version              :$VERSION"
echo "Binaries Directory   :$WOLFBIN"
echo "Building Single Player demo setup"
echo "==============================================="

# media
# NOTE TTimo: I maintain this directly in my local CVS
# module name is: WolfMedia-<version>
WOLFMEDIA=../../../../WolfMedia-SPdemo
BASEGAME=demomain

# location of the setup dir (for graphical installer and makeself)
# IMPORTANT NOTE: the same reference tree is used for both full and demo setups
SETUPDIR=setup-1.5.8-Id

# copy all the relevant data in the relevant places
prepare_core()
{
  echo "Cleaning up and rebuilding $TMPDIR"
  rm -rf $TMPDIR

  # binaries, copy and strip
  mkdir -p $TMPDIR/bin/x86
  cp $WOLFBIN/wolfsp.x86 $TMPDIR/bin/x86/wolfsp.x86
  strip $TMPDIR/bin/x86/wolfsp.x86
  brandelf -t Linux $TMPDIR/bin/x86/wolfsp.x86
  mkdir $TMPDIR/$BASEGAME
  cp $WOLFBIN/$BASEGAME/qagamei386.so $TMPDIR/$BASEGAME
  strip $TMPDIR/$BASEGAME/qagamei386.so
  cp $WOLFBIN/$BASEGAME/cgamei386.so $TMPDIR/$BASEGAME
  strip $TMPDIR/$BASEGAME/cgamei386.so
  cp $WOLFBIN/$BASEGAME/uii386.so $TMPDIR/$BASEGAME
  strip $TMPDIR/$BASEGAME/uii386.so

  # copy various accompagnying media files
  cp -R $WOLFMEDIA/* $TMPDIR
  # the demo is a bit tricky, we don't copy the main pak systematically
  # cp doesn't provide easy exclusion based on name
  # so we just copy in full and take out the pak0.pk3 afterwards (yuck)
  rm $TMPDIR/$BASEGAME/pak0.pk3
  
  # copy base setup files
  cp -R $SETUPDIR/setup.sh $SETUPDIR/setup.data $TMPDIR
  
  # make the installer linux friendly: use a symlink
  (
  cd $TMPDIR/setup.data/bin
  ln -s Linux FreeBSD
  ln -s Linux OpenBSD
  ln -s Linux NetBSD
  )

  # menu shortcut to the game
  # FIXME current setup doesn't have a way to set symlinks on arbitrary things
  # so we use a dummy script for each of the symlink targets
  # (scripts which will be overwritten by postinstall.sh)
  echo -e "#!/bin/sh\necho \"If you read this, then the setup script failed miserably.\nPlease report to ttimo@idsoftware.com\n\"" > $TMPDIR/bin/x86/wolfspdemo
  
  # remove CVS entries (always last thing to do, safer)  
  find $TMPDIR -name CVS | xargs rm -rf
}

  TMPDIR=wolf-setup-spdemo-nomedia  
  prepare_core
  # build the auto-extractible archive  
  ./$SETUPDIR/makeself/makeself.sh $TMPDIR wolfspdemo-linux-nomedia-$VERSION.x86.run "Return To Castle Wolfenstein Single Player DEMO (nomedia)" ./setup.sh $VERSION

  # stuff for building the full version remotely (faster than uploading it over DSL)
  SCRIPTDIR=build-full-setup-sp
  rm -rf $SCRIPTDIR
  mkdir $SCRIPTDIR
  cp ./$SETUPDIR/makeself/makeself.sh $SCRIPTDIR
  # make sure to watch this one
  echo -e "
  # this will build a full setup
  # the content needs to be in $TMPDIR
  # and copy the appropriate pk3 in there first
  FULLTMPDIR=wolf-setup-spdemo
  mv $TMPDIR \$FULLTMPDIR
  ./makeself.sh \$FULLTMPDIR wolfspdemo-linux-$VERSION.x86.run \"Return To Castle Wolfenstein Single Player DEMO\" ./setup.sh $VERSION
  " > $SCRIPTDIR/build.sh
  chmod +x $SCRIPTDIR/build.sh
  tar cvzf $SCRIPTDIR.tgz $SCRIPTDIR
  
