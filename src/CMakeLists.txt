
include_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${QT_INCLUDES}
)

link_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT_LIBRARY_DIR}
)

configure_file (cmake_config.h ${CMAKE_CURRENT_BINARY_DIR}/config.h)

configure_file (qjackctl.desktop ${CMAKE_CURRENT_BINARY_DIR}/qjackctl.desktop)

set (HEADERS
  qjackctl.h
  qjackctlAbout.h
  qjackctlAlsaConnect.h
  qjackctlAlsaGraph.h
  qjackctlConnect.h
  qjackctlAliases.h
  qjackctlGraph.h
  qjackctlGraphCommand.h
  qjackctlInterfaceComboBox.h
  qjackctlJackConnect.h
  qjackctlJackGraph.h
  qjackctlPatchbay.h
  qjackctlPatchbayFile.h
  qjackctlPatchbayRack.h
  qjackctlSession.h
  qjackctlSetup.h
  qjackctlStatus.h
  qjackctlSystemTray.h
  qjackctlAboutForm.h
  qjackctlConnectionsForm.h
  qjackctlGraphForm.h
  qjackctlMainForm.h
  qjackctlMessagesStatusForm.h
  qjackctlPatchbayForm.h
  qjackctlSessionForm.h
  qjackctlSetupForm.h
  qjackctlSocketForm.h
)

set (SOURCES
  qjackctl.cpp
  qjackctlAlsaConnect.cpp
  qjackctlAlsaGraph.cpp
  qjackctlConnect.cpp
  qjackctlAliases.cpp
  qjackctlGraph.cpp
  qjackctlGraphCommand.cpp
  qjackctlInterfaceComboBox.cpp
  qjackctlJackConnect.cpp
  qjackctlJackGraph.cpp
  qjackctlPatchbay.cpp
  qjackctlPatchbayFile.cpp
  qjackctlPatchbayRack.cpp
  qjackctlSession.cpp
  qjackctlSetup.cpp
  qjackctlSystemTray.cpp
  qjackctlAboutForm.cpp
  qjackctlConnectionsForm.cpp
  qjackctlGraphForm.cpp
  qjackctlMainForm.cpp
  qjackctlMessagesStatusForm.cpp
  qjackctlPatchbayForm.cpp
  qjackctlSessionForm.cpp
  qjackctlSetupForm.cpp
  qjackctlSocketForm.cpp
)

set (FORMS
  qjackctlAboutForm.ui
  qjackctlConnectionsForm.ui
  qjackctlGraphForm.ui
  qjackctlMainForm.ui
  qjackctlMessagesStatusForm.ui
  qjackctlPatchbayForm.ui
  qjackctlSessionForm.ui
  qjackctlSetupForm.ui
  qjackctlSocketForm.ui
)

set (RESOURCES
  qjackctl.qrc
)

set (TRANSLATIONS
	translations/qjackctl_cs.ts
	translations/qjackctl_de.ts
	translations/qjackctl_es.ts
	translations/qjackctl_fr.ts
	translations/qjackctl_it.ts
	translations/qjackctl_ja.ts
	translations/qjackctl_nl.ts
	translations/qjackctl_pt.ts
	translations/qjackctl_ru.ts
)

qt5_add_translation ( QM_FILES ${TRANSLATIONS} )
add_custom_target( translations ALL DEPENDS ${QM_FILES} )

qt5_wrap_ui (UI_SOURCES ${FORMS})
qt5_wrap_cpp (MOC_SOURCES ${HEADERS})
qt5_add_resources (QRC_SOURCES ${RESOURCES})

add_executable (qjackctl
  ${UI_SOURCES}
  ${MOC_SOURCES}
  ${QRC_SOURCES}
  ${SOURCES}
)

target_link_libraries (qjackctl PRIVATE Qt5::Widgets Qt5::Xml)

if (CONFIG_XUNIQUE)
  target_link_libraries (qjackctl PRIVATE Qt5::Network)
endif ()

if (CONFIG_DBUS)
  target_link_libraries (qjackctl PRIVATE Qt5::DBus)
endif ()

if (CONFIG_JACK)
  target_link_libraries (qjackctl PRIVATE ${JACK_LIBRARIES})
endif ()

if (CONFIG_ALSA_SEQ)
  target_link_libraries (qjackctl PRIVATE ${ALSA_LIBRARIES})
endif ()

if (CONFIG_PORTAUDIO)
  target_link_libraries (qjackctl PRIVATE ${PORTAUDIO_LIBRARIES})
endif ()


if (UNIX AND NOT APPLE)
  install (TARGETS qjackctl RUNTIME
     DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (FILES ${QM_FILES}
     DESTINATION ${CMAKE_INSTALL_DATADIR}/qjackctl/translations)
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/qjackctl.desktop
     DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install (FILES images/qjackctl.png
     DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
  install (FILES appdata/qjackctl.appdata.xml
     DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif ()
