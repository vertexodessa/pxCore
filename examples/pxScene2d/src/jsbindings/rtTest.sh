#!/bin/sh

export DYLD_LIBRARY_PATH=../../external/png/.libs/:../../external/curl/lib/.libs/:../../external/ft/objs/.libs/
export LD_LIBRARY_PATH=../../external/png/.libs/:../../external/jpg/.libs/:../../external/curl/lib/.libs/:../../external/libnode/out/Release/obj.target

[ -f FontdinerSwanky.ttf ] || cp ../FontdinerSwanky.ttf .
[ -f FreeSans.ttf ] || cp ../FreeSans.ttf .

echo $LD_LIBRARY_PATH
export NODE_PATH=../:`pwd`

#export NODE_DEBUG=/usr/lib/node_modules

#../rtNode load.js $*

#../rtNode start.js $*

#ulimit -s unlimited

echo
echo NOTE: Please use 'rtStart.sh' ... !!
echo

#../rtNode $1

#name=$1

#if [[ -n "$name" ]]; then
#    ../rtNode
#else
#    ../rtNode
#fi

#gdb ../rtNode

#valgrind --leak-check=summary  ../rtNode
