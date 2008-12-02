INCPATH += ../src

HEADERS += ../src/qjackctlAbout.h \
           ../src/qjackctlAlsaConnect.h \
           ../src/qjackctlConnect.h \
           ../src/qjackctlConnectAlias.h \
           ../src/qjackctlJackConnect.h \
           ../src/qjackctlPatchbay.h \
           ../src/qjackctlPatchbayFile.h \
           ../src/qjackctlPatchbayRack.h \
           ../src/qjackctlSetup.h \
           ../src/qjackctlStatus.h \
           ../src/qjackctlSystemTray.h \
           ../src/qjackctlAboutForm.h \
           ../src/qjackctlConnectionsForm.h \
           ../src/qjackctlMainForm.h \
           ../src/qjackctlMessagesForm.h \
           ../src/qjackctlPatchbayForm.h \
           ../src/qjackctlSetupForm.h \
           ../src/qjackctlSocketForm.h \
           ../src/qjackctlStatusForm.h

SOURCES += ../src/main.cpp \
           ../src/qjackctlAlsaConnect.cpp \
           ../src/qjackctlConnect.cpp \
           ../src/qjackctlConnectAlias.cpp \
           ../src/qjackctlJackConnect.cpp \
           ../src/qjackctlPatchbay.cpp \
           ../src/qjackctlPatchbayFile.cpp \
           ../src/qjackctlPatchbayRack.cpp \
           ../src/qjackctlSetup.cpp \
           ../src/qjackctlSystemTray.cpp \
           ../src/qjackctlAboutForm.cpp \
           ../src/qjackctlConnectionsForm.cpp \
           ../src/qjackctlMainForm.cpp \
           ../src/qjackctlMessagesForm.cpp \
           ../src/qjackctlPatchbayForm.cpp \
           ../src/qjackctlSetupForm.cpp \
           ../src/qjackctlSocketForm.cpp \
           ../src/qjackctlStatusForm.cpp

FORMS    = ../src/qjackctlAboutForm.ui \
           ../src/qjackctlConnectionsForm.ui \
           ../src/qjackctlMainForm.ui \
           ../src/qjackctlMessagesForm.ui \
           ../src/qjackctlPatchbayForm.ui \
           ../src/qjackctlSetupForm.ui \
           ../src/qjackctlSocketForm.ui \
           ../src/qjackctlStatusForm.ui

RESOURCES += ../icons/qjackctl.qrc


TEMPLATE = app
CONFIG  += qt thread warn_on release
LANGUAGE = C++

win32 {
	CONFIG  += console
	INCPATH += C:\usr\local\include
	LIBS    += -LC:\usr\local\lib
}

LIBS += -ljackmp

TRANSLATIONS = \
    ../translations/qjackctl_fr.ts \
    ../translations/qjackctl_es.ts \
    ../translations/qjackctl_ru.ts

# XML/DOM support
QT += xml
