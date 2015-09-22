#!/bin/sh

set -e
set -x

$linker $@ ../mingw/64/portaudio/lib/.libs/libportaudio.a ../weak_libjack.o -ldsound -luuid -lsetupapi -lole32 -lwinmm

#/home/kjetil/jack2/windows/Release64/bin/libportaudio_x86_64.a
#-Wl,--allow-multiple-definition
