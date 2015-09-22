#!/bin/sh

$@ ../weak_libjack.o ../mingw/32/libportaudio_x86.a  -ldsound -luuid -lsetupapi -lole32 -lwinmm

