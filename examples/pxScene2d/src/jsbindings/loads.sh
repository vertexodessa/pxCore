#!/bin/sh

export DYLD_LIBRARY_PATH=../../external/png/.libs/:../../external/curl/lib/.libs/:../../external/ft/objs/.libs/
export LD_LIBRARY_PATH=../../external/png/.libs/:../../external/jpg/.libs/:../../external/curl/lib/.libs/:../../external/libnode/out/Release/obj.target

[ -f FontdinerSwanky.ttf ] || cp ../FontdinerSwanky.ttf .
[ -f FreeSans.ttf ] || cp ../FreeSans.ttf .

echo $LD_LIBRARY_PATH

../pxNode load.js $*

#../rtNode load.js $*
