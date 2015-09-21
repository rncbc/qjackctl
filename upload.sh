#!/bin/sh

rm -f qjackctl.zip
zip -r qjackctl.zip mingw32 mingw64
scp qjackctl.zip ksvalast@login.uio.no:www_docs/
