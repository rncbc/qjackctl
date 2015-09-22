#!/bin/sh

if [[ $1 == "32" ]] ; then
    MINGW=mingw32
    CC=i686-w64-mingw32-gcc
    CXX=i686-w64-mingw32-g++
    echo "It is not necessary to build a 32 bit version of portaudio."
    exit -1
fi

if [[ $1 == "64" ]] ; then
   MINGW=mingw64
   CC=x86_64-w64-mingw32-gcc
   CXX=x86_64-w64-mingw32-g++
fi

if [ -z $MINGW ] ; then
    echo "Must either run \"$0 32\" (for 32 bit build) or  \"$0 64\" (for 64 bit build)"
    exit -1
fi

set -e
set -x

if [ ! -f $1 ] ; then
    mkdir $1
fi

cd $1

if [ ! -f portaudio ] ; then
    tar xvzf ../pa_stable_v19_20140130.tgz
fi

cd portaudio

if [ -f Makefile ] ; then
    $MINGW-make clean
fi

if [ ! -f configure ] ; then
    autoreconf -if
fi


# The asio sdk can be found here: http://www.steinberg.net/en/company/developers.html

$MINGW-configure --without-alsa --without-jack --with-asiodir=$HOME/SDKs/ASIOSDK2.3 --with-winapi=wmme,directx,asio
# wdmks
# (wasapi doesn't compile)

$MINGW-make

