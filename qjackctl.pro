SOURCES	+= main.cpp \
	qjackctlPatchbay.cpp
HEADERS	+= qjackctlPatchbay.h
FORMS	= qjackctlMainForm.ui
IMAGES	= qjackctl.xpm \
	start1.png \
	stop1.png \
	quit1.png \
	qtlogo.png \
	play1.png \
	pause1.png
TEMPLATE	=app
CONFIG	+= qt warn_on release
LIBS	+= -ljack
LANGUAGE	= C++
