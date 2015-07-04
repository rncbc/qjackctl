#!/bin/sh


########## CONFIGURE
###################################3

cp mingw32/mingw32_config.h src/config.h

patch -p1 <mingw32/diff.diff

./autogen.sh

mingw32-qmake-qt4




######### BUILD
####################################

EXTRAFLAGS="-I../mingw32/weakjack -I../mingw32/include -I../mingw32/portaudio/include  -DNO_JACK_METADATA -DUSE_WEAK_JACK"
EXTRALIBS="../mingw32/weak_libjack.o ../mingw32/portaudio/portaudio.dll"

mingw32-make CC="i686-w64-mingw32-gcc $EXTRAFLAGS" CXX="i686-w64-mingw32-gcc $EXTRAFLAGS" LINK="i686-w64-mingw32-g++ $EXTRALIBS" 





######### DIST
###################################

rm -fr release

mkdir release

cp src/release/qjackctl.exe release/
cp mingw32/portaudio/portaudio.dll release/

cp /usr/i686-w64-mingw32/sys-root/mingw/bin/QtCore4.dll release/
cp /usr/i686-w64-mingw32/sys-root/mingw/bin/QtGui4.dll release/
cp /usr/i686-w64-mingw32/sys-root/mingw/bin/QtXml4.dll release/

cp /usr/i686-w64-mingw32/sys-root/mingw/bin/libgcc_s_sjlj-1.dll release/
cp /usr/i686-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll release/
cp /usr/i686-w64-mingw32/sys-root/mingw/bin/zlib1.dll release/
cp /usr/i686-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll release/
cp /usr/i686-w64-mingw32/sys-root/mingw/bin/libpng16-16.dll release/
