#!/bin/sh
# quick hacky script to make my life easier

DIR=debug-x86-Linux-2.2-light/out

echo -e "cd $DIR; export DISPLAY=spoutnik:0.0; gvd wolfded.x86 --pargs +set fs_basepath /usr/local/games/wolfenstein-dedicated +set fs_debug 1 +set developer 1 +set sv_pure 1 +set dedicated 2 +set g_gametype 7 +map mp_trenchtoast" > .debug.sh.tmp

chmod +x .debug.sh.tmp
xterm -e "./.debug.sh.tmp"

