SOURCES	+= main.cpp \
	qjackctlPatchbay.cpp
HEADERS	+= qjackctlPatchbay.h
FORMS	= qjackctlMainForm.ui
IMAGES	= start1.png \
	stop1.png \
	quit1.png \
	qjackctl.xpm
TEMPLATE	=app
CONFIG	+= qt warn_on release
LIBS	+= -ljack
LANGUAGE	= C++
