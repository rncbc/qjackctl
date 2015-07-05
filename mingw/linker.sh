#!/bin/sh

$@ ../weak_libjack.o ../libportaudio.a  -ldsound -luuid -lsetupapi -lole32 -lwinmm
