#!/bin/sh

if [[ $1 == "32" ]] ; then
    MINGW=mingw32
    CC=i686-w64-mingw32-gcc
    CXX=i686-w64-mingw32-g++
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


function clean_and_configure {
    echo "*** Cleaning and configuring"
    
    rm -f is_configured
    
    $MINGW-make clean
    
    make -f Makefile.git clean
    ./autogen.sh

    $MINGW-qmake-qt4

    cp mingw/mingw_config.h src/config.h

    touch is_configured
}



########## PATCH
###################################

if [ ! -f is_patched ] ; then
    echo "*** Patching source"
    patch -p1 <mingw/diff.diff
    touch is_patched
fi


if ! grep "CONFIG+=static" src/src.pro ; then
    echo "CONFIG+=static" >>src/src.pro
fi




########## CONFIGURE
###################################

if [ ! -f src/config.h ] ; then
    clean_and_configure
fi

if [[ `diff mingw/mingw_config.h src/config.h` ]] ; then
    clean_and_configure
fi

if [ -f mingw_last_build_type.txt ] ; then
    if [[ ! $1 == `cat mingw_last_build_type.txt` ]] ; then
        clean_and_configure
    fi
fi

echo $1 >mingw_last_build_type.txt


if [ ! -f is_configured ] ; then
    clean_and_configure
fi



######### BUILD
####################################

EXTRAFLAGS="-I`pwd`/mingw/weakjack -I`pwd`/mingw/include -I`pwd`/mingw/$1/portaudio/include -DNO_JACK_METADATA -DUSE_WEAK_JACK"

rm -fr $MINGW
mkdir $MINGW

$CC $EXTRAFLAGS mingw/weakjack/weak_libjack.c -Wall -c -O2 -o weak_libjack.o
cp mingw/$1/portaudio/lib/.libs/libportaudio.a .

$MINGW-make -j8 CC="$CC $EXTRAFLAGS" CXX="$CXX $EXTRAFLAGS" LINK="../mingw/linker.sh $CXX"





######### DIST
###################################

cp src/release/qjackctl.exe $MINGW/
mingw-strip $MINGW/qjackctl.exe

