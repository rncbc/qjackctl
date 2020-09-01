set(NAME qjackctl)

include_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

configure_file (cmake_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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
  qjackctlPaletteForm.h
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
  qjackctlPaletteForm.cpp
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
  qjackctlPaletteForm.ui
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

if (WIN32)
  set (RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/win32/${NAME}.rc)
  set (RES_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.res.obj)
  find_program (WINDRES_EXECUTABLE NAMES windres mingw32-windres i686-mingw32-windres)
  if (MINGW)
    exec_program (${WINDRES_EXECUTABLE}
      ARGS "-i ${RC_FILE} -o ${RES_FILE} --include-dir=${CMAKE_CURRENT_SOURCE_DIR}/images")
    list (APPEND SOURCES ${RES_FILE})
  else ()
    list (APPEND SOURCES ${RC_FILE})
  endif ()
endif ()

if (APPLE)
  set (ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/images/${NAME}.icns)
  list (APPEND SOURCES ${ICON_FILE})
  set (MACOSX_BUNDLE_ICON_FILE ${NAME}.icns)
  set_source_files_properties (${ICON_FILE} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
endif ()


add_executable (${NAME}
  ${UI_SOURCES}
  ${MOC_SOURCES}
  ${QRC_SOURCES}
  ${SOURCES}
)

set_target_properties (${NAME} PROPERTIES CXX_STANDARD 17)

if (WIN32)
  set_target_properties (${NAME} PROPERTIES WIN32_EXECUTABLE true)
endif ()

if (APPLE)
  set_target_properties (${NAME} PROPERTIES MACOSX_BUNDLE true)
endif ()

target_link_libraries (${NAME} PRIVATE Qt5::Widgets Qt5::Xml)

if (CONFIG_XUNIQUE)
  target_link_libraries (${NAME} PRIVATE Qt5::Network)
endif ()

if (CONFIG_DBUS)
  target_link_libraries (${NAME} PRIVATE Qt5::DBus)
endif ()

if (CONFIG_JACK)
  target_link_libraries (${NAME} PRIVATE ${JACK_LIBRARIES})
endif ()

if (CONFIG_ALSA_SEQ)
  target_link_libraries (${NAME} PRIVATE ${ALSA_LIBRARIES})
endif ()

if (CONFIG_PORTAUDIO)
  target_link_libraries (${NAME} PRIVATE ${PORTAUDIO_LIBRARIES})
endif ()


if (UNIX AND NOT APPLE)
  install (TARGETS ${NAME} RUNTIME
     DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (FILES ${QM_FILES}
     DESTINATION ${CMAKE_INSTALL_DATADIR}/${NAME}/translations)
  install (FILES ${NAME}.desktop
     DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install (FILES images/${NAME}.png
     DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
  install (FILES images/${NAME}.svg
     DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
  install (FILES appdata/${NAME}.appdata.xml
     DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif ()

if (WIN32)
   install (TARGETS ${NAME} RUNTIME
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
   install (FILES ${QM_FILES}
     DESTINATION ${CMAKE_INSTALL_DATADIR}/${NAME}/translations)
endif ()

