SOURCES	+= main.cpp
unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
FORMS	= qjackctlMainForm.ui
IMAGES	= start1.png \
	stop1.png \
	quit1.png \
	qjackctl.xpm
TEMPLATE	=app
CONFIG	+= qt warn_on release
LANGUAGE	= C++
