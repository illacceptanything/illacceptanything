#!/bin/bash

usage()
{
echo "syntax: build_setup.sh [<config>] <setup directory> <single player directory> <version>"
echo "  (ex: build_setup.sh ../../../WolfSP/src/unix/release-x86-Linux-2.1/out release-x86-Linux-2.1/out 0.7.16-1)"
echo "syntax: build_setup.sh demo <setup_directory> <version>"
echo "  builds an MP demo version setup"
echo "syntax: build_setup.sh light <setup_directory> <version>"
echo "  builds a light dedicated server setup"
echo "syntax: build_setup.sh full <setup directory> <single player directory> <version>"
echo "  builds the full setup (default is the -GOTY setup)"
}

check_brandelf()
{
  # make sure brandelf is installed to avoid any problem when building the setups
  # NOTE: when cons spawns this, the environement variables are greatly restricted (which makes things very annoying)
  BRAND=`which brandelf`;
  if [ -n "$BRAND" ] && [ -x "$BRAND" ]
  then
    echo "brandelf is present: $BRAND"
  else
    #export
    BRAND=/home/timo/usr/bin/brandelf;
    if [ -n "$BRAND" ] && [ -x "$BRAND" ]
    then
      echo "brandelf is present: $BRAND"
    else
      echo "brandelf not found"
      exit
    fi
  fi
}

# safe checks
check_brandelf

DO_DEMO=0
DO_LIGHT=0
DO_FULL=0
DO_INCREMENTAL=0

# process command line
if [ $# -ne 3 ] && [ $# -ne 4 ]
then
  echo "bad options"
  usage
  exit
fi
if [ $1 == "demo" ]
then
  DO_DEMO=1
  WOLFBIN=$2
  VERSION=$3
elif [ $1 == "light" ]
then
  DO_LIGHT=1
  WOLFBIN=$2
  VERSION=$3
elif [ $1 == "full" ]
then
  DO_FULL=1
  WOLFBIN=$2
  WOLFSPBIN=$3
  VERSION=$4
elif [ $1 == "incremental" ]
then
  DO_INCREMENTAL=1
  WOLFBIN=$2
  WOLFSPBIN=$3
  VERSION=$4    
else
  WOLFBIN=$1
  WOLFSPBIN=$2
  VERSION=$3
fi

echo "Building setup ================================"
echo   "Version                 :$VERSION"
echo   "Multiplayer directory   :$WOLFBIN"
if [ $DO_DEMO -eq 1 ]
then
  echo "Building Mutliplayer demo setup"
elif [ $DO_LIGHT -eq 1 ]
then
  echo "Building Light dedicated server"
else
  if [ $DO_FULL -eq 1 ]
  then
    echo "Building Full setup"
  else
    echo "Building GOTY setup"
  fi
  echo "  Single Player directory: $WOLFSPBIN"
fi
echo "==============================================="

# media
# NOTE TTimo: I maintain this directly in my local CVS
# module name is: WolfMedia-<version> (<version> == main or demo)
WOLFMEDIA_LINUX=../../../../WolfMedia-dedicated-linux
WOLFMEDIA_FULL=../../../../WolfMedia-GOTY-maps
if [ $DO_DEMO -eq 1 ]
then
  WOLFMEDIA=../../../../WolfMedia-demo
  BASEGAME=demomain
elif [ $DO_LIGHT -eq 1 ]
then
  WOLFMEDIA=../../../../WolfMedia-dedicated
  BASEGAME=main
else
  WOLFMEDIA=../../../../WolfMedia-main
  BASEGAME=main
fi

# location of the setup dir (for graphical installer and makeself)
# IMPORTANT NOTE: the same reference tree is used for both full and demo setups
SETUPDIR=setup

# copy all the relevant data in the relevant places
prepare_core()
{
  echo "Cleaning up and rebuilding $TMPDIR"
  rm -rf $TMPDIR

  # binaries, copy and strip
  mkdir -p $TMPDIR/bin/x86
  if [ $DO_LIGHT -ne 1 ]
  then
    cp $WOLFBIN/wolf.x86 $TMPDIR/bin/x86/wolf.x86
    strip $TMPDIR/bin/x86/wolf.x86
    $BRAND -t Linux $TMPDIR/bin/x86/wolf.x86
  fi
  cp $WOLFBIN/wolfded.x86 $TMPDIR/bin/x86/wolfded.x86
  strip $TMPDIR/bin/x86/wolfded.x86
  $BRAND -t Linux $TMPDIR/bin/x86/wolfded.x86
  mkdir $TMPDIR/$BASEGAME
  cp $WOLFBIN/$BASEGAME/qagame.mp.i386.so $TMPDIR/$BASEGAME
  strip $TMPDIR/$BASEGAME/qagame.mp.i386.so
  if [ $DO_LIGHT -ne 1 ]
  then
    cp $WOLFBIN/$BASEGAME/cgame.mp.i386.so $TMPDIR/$BASEGAME
    strip $TMPDIR/$BASEGAME/cgame.mp.i386.so
    cp $WOLFBIN/$BASEGAME/ui.mp.i386.so $TMPDIR/$BASEGAME
    strip $TMPDIR/$BASEGAME/ui.mp.i386.so
  fi
  
  # punkbuster
  mkdir $TMPDIR/pb
  cp $WOLFBIN/pb/*.so $TMPDIR/pb
  strip $TMPDIR/pb/*.so
  # punkbuster html files
  mkdir $TMPDIR/pb/htm
  cp ../pb/htm/* $TMPDIR/pb/htm

  # copy various accompagnying media files
  cp $CPOPT -R $WOLFMEDIA/* $TMPDIR
  if [ $DO_DEMO -eq 1 ]
  then
    # the demo is a bit tricky, we don't copy the main pak systematically
    # cp doesn't provide easy exclusion based on name
    # so we just copy in full and take out the pak0.pk3 afterwards (yuck)
    rm $TMPDIR/$BASEGAME/pak0.pk3
  fi
  
  if [ $DO_DEMO -eq 0 ]
  then
    echo "Copying mp_bin.pk3 from CVS"
    # mp_bin.pk3:
    # grab from the CVS, that's the reference location
    cp ../../MAIN/mp_bin.pk3 $TMPDIR/$BASEGAME
    md5sum $TMPDIR/$BASEGAME/mp_bin.pk3
  fi
  
  # copy base setup files
  cp $CPOPT -R $SETUPDIR/setup.sh $SETUPDIR/setup.data $TMPDIR
  
  # copy the full/demo specific files
  if [ $DO_DEMO -eq 1 ]
  then
    cp $SETUPDIR/setup.data.demo/* $TMPDIR/setup.data
  elif [ $DO_LIGHT -eq 1 ]
  then
    cp $SETUPDIR/setup.data.light/* $TMPDIR/setup.data
  else
    cp $SETUPDIR/setup.data.full/* $TMPDIR/setup.data
  fi    

  # make the installer BSD friendly: use a symlink
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
  if [ $DO_DEMO -eq 1 ]  
  then  
    echo -e "#!/bin/sh\necho \"If you read this, then the setup script failed miserably.\nPlease report to ttimo@idsoftware.com\n\"" > $TMPDIR/bin/x86/wolfmpdemo
  elif [ $DO_LIGHT -eq 1 ]
  then
    echo -e "#!/bin/sh\necho \"If you read this, then the setup script failed miserably.\nPlease report to ttimo@idsoftware.com\n\"" > $TMPDIR/bin/x86/wolflightded
  else
    echo -e "#!/bin/sh\necho \"If you read this, then the setup script failed miserably.\nPlease report to ttimo@idsoftware.com\n\"" > $TMPDIR/bin/x86/wolfmp
  fi
  
  # remove CVS entries (always last thing to do, safer)  
  find $TMPDIR -name CVS | xargs rm -rf
}

# single player specific files
prepare_spcore()
{
  cp $WOLFSPBIN/wolfsp.x86 $TMPDIR/bin/x86/wolfsp.x86
  strip $TMPDIR/bin/x86/wolfsp.x86
  $BRAND -t Linux $TMPDIR/bin/x86/wolfsp.x86
  cp $WOLFSPBIN/$BASEGAME/qagamei386.so $TMPDIR/$BASEGAME
  strip $TMPDIR/$BASEGAME/qagamei386.so
  cp $WOLFSPBIN/$BASEGAME/cgamei386.so $TMPDIR/$BASEGAME
  strip $TMPDIR/$BASEGAME/cgamei386.so
  cp $WOLFSPBIN/$BASEGAME/uii386.so $TMPDIR/$BASEGAME
  strip $TMPDIR/$BASEGAME/uii386.so  
  
  # dummy symlink script
  echo -e "#!/bin/sh\necho \"If you read this, then the setup script failed miserably.\nPlease report to ttimo@idsoftware.com\n\"" > $TMPDIR/bin/x86/wolfsp
}

# cp setup phase
# we need to copy the symlinked files, and not the symlinks themselves
# on antares this is forced with a cp -L
# on spoutnik, -L is not recognized, and dereference is the default behaviour
# we need a robust way of checking
TESTFILE=/tmp/foo$$
touch $TESTFILE
# see if option is supported
cp -L $TESTFILE $TESTFILE.cp 2>/dev/null
if [ $? -eq 1 ]
then
  # option not supported, should be on by default
  echo "cp doesn't have -L option"
  unset CPOPT
else
  # option supported, use it
  echo "cp supports -L option"
  CPOPT="-L"
fi
rm $TESTFILE

if [ $DO_DEMO -eq 1 ]
then

  TMPDIR=wolf-setup-mpdemo-nomedia  
  prepare_core
  # build the auto-extractible archive  
  ./$SETUPDIR/makeself/makeself.sh $TMPDIR wolfmpdemo-linux-nomedia-$VERSION.x86.run "Return To Castle Wolfenstein Multiplayer DEMO (nomedia)" ./setup.sh $VERSION

  # stuff for building the full version remotely (faster than uploading it over DSL)
  SCRIPTDIR=build-full-setup
  rm -rf $SCRIPTDIR
  mkdir $SCRIPTDIR
  cp ./$SETUPDIR/makeself/makeself.sh $SCRIPTDIR
  # make sure to watch this one
  echo -e "
  # this will build a full setup
  # the content needs to be in $TMPDIR
  # and copy the appropriate pk3 in there first
  FULLTMPDIR=wolf-setup-mpdemo
  mv $TMPDIR \$FULLTMPDIR
  ./makeself.sh \$FULLTMPDIR wolfmpdemo-linux-$VERSION.x86.run \"Return To Castle Wolfenstein Multiplayer DEMO\" ./setup.sh $VERSION
  " > $SCRIPTDIR/build.sh
  chmod +x $SCRIPTDIR/build.sh
  tar cvzf $SCRIPTDIR.tgz $SCRIPTDIR

elif [ $DO_LIGHT -eq 1 ]
then

  TMPDIR=wolf-setup-lightded
  prepare_core
  # deal with the second linux only dir
  cp $CPOPT -R $WOLFMEDIA_LINUX/* $TMPDIR  
  # remove CVS entries (always last thing to do, safer)  
  find $TMPDIR -name CVS | xargs rm -rf
  # dump the command to consol
  echo "./$SETUPDIR/makeself/makeself.sh $TMPDIR wolflightded-linux-$VERSION.x86.run \"Return To Castle Wolfenstein - Standalone Dedicated Server\" ./setup.sh $VERSION"
  # build the auto-extractible archive
  ./$SETUPDIR/makeself/makeself.sh $TMPDIR wolflightded-linux-$VERSION.x86.run "Return To Castle Wolfenstein - Standalone Dedicated Server" ./setup.sh $VERSION
  
elif [ $DO_FULL -eq 1 ]  
then

  # could have unified with above, kept seperate for readability
  TMPDIR=wolf-setup-full
  prepare_core
  prepare_spcore
  # copy content that GOTY provides
  cp $CPOPT -R $WOLFMEDIA_FULL/* $TMPDIR
  # dump the command to the console, so that it is easy to build manual auto-update files
  echo "./$SETUPDIR/makeself/makeself.sh $TMPDIR wolf-linux-$VERSION-full.x86.run \"Return To Castle Wolfenstein\" ./setup.sh $VERSION"
  # build the auto-extractible archive
  ./$SETUPDIR/makeself/makeself.sh $TMPDIR wolf-linux-$VERSION-full.x86.run "Return To Castle Wolfenstein" ./setup.sh $VERSION

elif [ $DO_INCREMENTAL -eq 1 ]
then
  
  # incremental, this needs to be adapted for each major / incremental release
  INCR_FROM=1.4
  TMPDIR=wolf-setup-incr
  prepare_core
  prepare_spcore
	# 1.33 -> 1.4
  ## do the incremental stuff
  #cp $CPOPT $WOLFMEDIA_FULL/main/sp_pak3.pk3 $WOLFMEDIA_FULL/main/sp_pak4.pk3 $TMPDIR/main/
  ## build the auto-extractible archive
  #echo "./$SETUPDIR/makeself/makeself.sh $TMPDIR wolf-linux-up-$INCR_FROM-$VERSION.x86.run \"Return To Castle Wolfenstein Incremental Update $INCR_FROM $VERSION\" ./setup.sh $VERSION"
  #./$SETUPDIR/makeself/makeself.sh $TMPDIR wolf-linux-up-$INCR_FROM-$VERSION.x86.run "Return To Castle Wolfenstein Incremental Update $INCR_FROM $VERSION" ./setup.sh $VERSION
	
	# 1.4 -> 1.41
	WOLFMEDIA_UPDATE=../../../../WolfMedia-1.41
	# go back and only put the stuff we actually want
	rm -rf $TMPDIR/$BASEGAME
	rm -rf $TMPDIR/Docs/Help
	rm -rf $TMPDIR/Docs/PunkBuster
	mkdir $TMPDIR/$BASEGAME
  cp $WOLFBIN/$BASEGAME/qagame.mp.i386.so $TMPDIR/$BASEGAME
  strip $TMPDIR/$BASEGAME/qagame.mp.i386.so	
	cp $CPOPT -R $WOLFMEDIA_UPDATE/* $TMPDIR	
  # build the auto-extractible archive
  echo "./$SETUPDIR/makeself/makeself.sh $TMPDIR wolf-linux-update-$VERSION.x86.run \"Return To Castle Wolfenstein Update $VERSION\" ./setup.sh $VERSION"
  ./$SETUPDIR/makeself/makeself.sh $TMPDIR wolf-linux-update-$VERSION.x86.run "Return To Castle Wolfenstein Update $VERSION" ./setup.sh $VERSION
    
else

  TMPDIR=wolf-setup
  prepare_core
  # the single player specific stuff
  prepare_spcore
  # dump the command to the console, so that it is easy to build manual auto-update files
  echo "./$SETUPDIR/makeself/makeself.sh $TMPDIR wolf-linux-$VERSION-GOTY.x86.run \"Return To Castle Wolfenstein\" ./setup.sh $VERSION"
  # build the auto-extractible archive
  ./$SETUPDIR/makeself/makeself.sh $TMPDIR wolf-linux-$VERSION-GOTY.x86.run "Return To Castle Wolfenstein" ./setup.sh $VERSION
  
fi
