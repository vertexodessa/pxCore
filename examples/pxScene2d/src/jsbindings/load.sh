#!/bin/sh

export LD_LIBRARY_PATH=../../external/png/.libs/:../../external/jpg/.libs/

[ -f FontdinerSwanky.ttf ] || cp ../FontdinerSwanky.ttf .
[ -f FreeSans.ttf ] || cp ../FreeSans.ttf .

echo $LD_LIBRARY_PATH

#valgrind node load.js $*

node load.js $*
