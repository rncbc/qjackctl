#!/bin/sh

if [[ $1 == "32" ]] ; then
    MINGW=mingw32
    CC=i686-w64-mingw32-gcc
    CXX=i686-w64-mingw32-g++
fi

if [[ $1 == "64" ]] ; then
   MINGW=mingw64
   CC=x86_64-w64-mingw32-gcc
   CXX=x86_64-w64-mingw32-gcc
fi

if [ -z $MINGW ] ; then
    echo "Must either run \"$0 32\" (for 32 bit build) or  \"$0 64\" (for 64 bit build)"
    exit -1
fi

set -e
set -x

cd $1/portaudio

$MINGW-make clean

$MINGW-configure --without-alsa --without-jack --with-asiodir=$HOME/SDKs/ASIOSDK2.3 --with-winapi=wmme,directx,asio,wmme,wdmks
# (wasapi doesn't compile)

$MINGW-make

