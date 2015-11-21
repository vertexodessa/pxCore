#!/bin/sh

pxSceneDir=../..
jsbindingsDir=$pxSceneDir/src/jsbindings

echo "Start"
echo "pxSceneDir=" $pxSceneDir
echo "jsbindingsDir=" $jsbindingsDir
echo "\n"

export DYLD_LIBRARY_PATH=$pxSceneDir/external/png/.libs/:$pxSceneDir/external/curl/lib/.libs/:$pxSceneDir/external/ft/objs/.libs/
export LD_LIBRARY_PATH=$pxSceneDir/external/png/.libs/:$pxSceneDir/external/jpg/.libs/:$pxSceneDir/external/curl/lib/.libs/:../../external/libnode/out/Release/obj.target
export NODE_PATH=./:$jsbindingsDir/build/Debug:./node_modules

[ -f FontdinerSwanky.ttf ] || cp $pxSceneDir/src/FontdinerSwanky.ttf .
[ -f FreeSans.ttf ] || cp $pxSceneDir/src/FreeSans.ttf .

echo  LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH

export NODE_PATH=$NODE_PATH:`pwd`

echo  NODE_PATH
echo $NODE_PATH

../rtNode

##gdb --args ../rtNode start.js url=$*
#node start.js url=$*
