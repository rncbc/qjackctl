SOURCES	+= main.cpp
FORMS	= qjackctlMainForm.ui
IMAGES	= start1.png \
	stop1.png \
	quit1.png \
	qjackctl.xpm
TEMPLATE	=app
CONFIG	+= qt warn_on release
LANGUAGE	= C++
LIBS += -ljack
