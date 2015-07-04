#!/bin/sh

i686-w64-mingw32-g++ $@ ../mingw32/weak_libjack.o ../mingw32/portaudio/lib/.libs/libportaudio.a  -ldsound -luuid -lsetupapi -lole32 -lwinmm
