# qjackctl.pro
#
TARGET = qjackctl

TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

include(src.pri)

#DEFINES += DEBUG

HEADERS += config.h \
	qjackctlAbout.h \
	qjackctlAlsaConnect.h \
	qjackctlConnect.h \
	qjackctlConnectAlias.h \
	qjackctlJackConnect.h \
	qjackctlPatchbay.h \
	qjackctlPatchbayFile.h \
	qjackctlPatchbayRack.h \
	qjackctlSession.h \
	qjackctlSetup.h \
	qjackctlStatus.h \
	qjackctlSystemTray.h \
	qjackctlAboutForm.h \
	qjackctlConnectionsForm.h \
	qjackctlMainForm.h \
	qjackctlMessagesForm.h \
	qjackctlPatchbayForm.h \
	qjackctlSessionForm.h \
	qjackctlSetupForm.h \
	qjackctlSocketForm.h \
	qjackctlStatusForm.h

SOURCES += \
	qjackctl.cpp \
	qjackctlAlsaConnect.cpp \
	qjackctlConnect.cpp \
	qjackctlConnectAlias.cpp \
	qjackctlJackConnect.cpp \
	qjackctlPatchbay.cpp \
	qjackctlPatchbayFile.cpp \
	qjackctlPatchbayRack.cpp \
	qjackctlSession.cpp \
	qjackctlSetup.cpp \
	qjackctlSystemTray.cpp \
	qjackctlAboutForm.cpp \
	qjackctlConnectionsForm.cpp \
	qjackctlMainForm.cpp \
	qjackctlMessagesForm.cpp \
	qjackctlPatchbayForm.cpp \
	qjackctlSessionForm.cpp \
	qjackctlSetupForm.cpp \
	qjackctlSocketForm.cpp \
	qjackctlStatusForm.cpp

FORMS += \
	qjackctlAboutForm.ui \
	qjackctlConnectionsForm.ui \
	qjackctlMainForm.ui \
	qjackctlMessagesForm.ui \
	qjackctlPatchbayForm.ui \
	qjackctlSessionForm.ui \
	qjackctlSetupForm.ui \
	qjackctlSocketForm.ui \
	qjackctlStatusForm.ui

RESOURCES += \
	qjackctl.qrc

TRANSLATIONS += \
	translations/qjackctl_cs.ts \
	translations/qjackctl_de.ts \
	translations/qjackctl_es.ts \
	translations/qjackctl_fr.ts \
	translations/qjackctl_it.ts \
	translations/qjackctl_ru.ts

unix {

	#VARIABLES
	OBJECTS_DIR = .obj
	MOC_DIR     = .moc
	UI_DIR      = .ui

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	BINDIR = $$PREFIX/bin
	DATADIR = $$PREFIX/share

	DEFINES += DATADIR=\"$$DATADIR\" PKGDATADIR=\"$$PKGDATADIR\"

	#MAKE INSTALL
	INSTALLS += target desktop icon

	target.path = $$BINDIR

	desktop.path = $$DATADIR/applications
	desktop.files += $${TARGET}.desktop

	icon.path = $$DATADIR/icons/hicolor/32x32/apps
	icon.files += images/$${TARGET}.png 
}

# XML/DOM support
QT += xml
