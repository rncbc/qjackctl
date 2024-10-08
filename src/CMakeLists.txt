# project (qjackctl)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
  file (REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
endif ()
configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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
  qjackctlSessionSaveForm.h
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
  qjackctlSessionSaveForm.cpp
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
  qjackctlSessionSaveForm.ui
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
	translations/qjackctl_ko.ts
	translations/qjackctl_nl.ts
	translations/qjackctl_pt_BR.ts
	translations/qjackctl_ru.ts
	translations/qjackctl_sk.ts
	translations/qjackctl_tr.ts
	translations/qjackctl_uk.ts
)

if (QT_VERSION VERSION_LESS 5.15.0)
  qt5_add_translation (QM_FILES ${TRANSLATIONS})
else ()
  qt_add_translation (QM_FILES ${TRANSLATIONS})
endif ()

add_custom_target (translations ALL DEPENDS ${QM_FILES})

add_executable (${PROJECT_NAME}
  ${HEADERS}
  ${SOURCES}
  ${FORMS}
  ${RESOURCES}
)

# Add some debugger flags.
if (CONFIG_DEBUG AND UNIX AND NOT APPLE)
  set (CONFIG_DEBUG_OPTIONS -g -fsanitize=address -fno-omit-frame-pointer)
  target_compile_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
  target_link_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
endif ()

set_target_properties (${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

if (WIN32)
  set_target_properties (${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE true)
  target_compile_definitions (${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES)
endif ()

if (APPLE)
  set_target_properties (${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE true)
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Svg)

if (CONFIG_XUNIQUE)
  target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Network)
endif ()

if (CONFIG_DBUS)
  target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::DBus)
endif ()

if (CONFIG_JACK)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::JACK)
endif ()

if (CONFIG_ALSA_SEQ)
  target_link_libraries (${PROJECT_NAME} PRIVATE ALSA::ALSA)
endif ()

if (CONFIG_PORTAUDIO)
  target_link_libraries (${PROJECT_NAME} PRIVATE PortAudio::PortAudio)
endif ()

if (CONFIG_COREAUDIO)
  target_link_libraries (${PROJECT_NAME} PRIVATE ${CORE_FOUNDATION_LIBRARY} ${CORE_AUDIO_LIBRARY})
endif ()


if (MINGW OR (UNIX AND NOT APPLE))
  install (TARGETS ${PROJECT_NAME} RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (FILES ${QM_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations)
  install (FILES images/${PROJECT_NAME}.png
    RENAME org.rncbc.${PROJECT_NAME}.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
  install (FILES images/${PROJECT_NAME}.svg
    RENAME org.rncbc.${PROJECT_NAME}.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
  install (FILES appdata/org.rncbc.${PROJECT_NAME}.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install (FILES appdata/org.rncbc.${PROJECT_NAME}.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
  install (FILES man1/${PROJECT_NAME}.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
  install (FILES man1/${PROJECT_NAME}.fr.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/fr/man1 RENAME ${PROJECT_NAME}.1)
  install (FILES palette/KXStudio.conf palette/Wonton\ Soup.conf
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/palette)
elseif (APPLE)
  # Generate bundle on MacOS with QjackCtl.icns as the icon and a custom Info.plist file.
  target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/images/QjackCtl.icns)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/images/QjackCtl.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE True
    OUTPUT_NAME QjackCtl
    MACOSX_RPATH ON
    INSTALL_RPATH @executable_path/../Frameworks
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/appdata/Info.plist
  )
  install (TARGETS ${PROJECT_NAME} BUNDLE
     DESTINATION ${CMAKE_INSTALL_BINDIR})
elseif (WIN32)
  install (TARGETS ${PROJECT_NAME} RUNTIME
     DESTINATION .)
  # Use same path as windeployqt for translations
  install (FILES ${QM_FILES}
     DESTINATION translations)
endif ()


if (WIN32 AND CONFIG_INSTALL_QT)
  get_target_property (_qt_qmake_location Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
  get_filename_component (_qt_bin_dir "${_qt_qmake_location}" DIRECTORY)
  set (QT_WINDEPLOYQT "${_qt_bin_dir}/windeployqt.exe")
  if (EXISTS ${QT_WINDEPLOYQT})
    # execute windeployqt in a tmp directory after build
    if (QT_VERSION_MAJOR MATCHES "5")
      # These options are not available with Qt6
      set (WINDEPLOYQT_EXTRA_OPTIONS --no-webkit2 --no-angle)
    endif ()
    add_custom_command (TARGET ${PROJECT_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
      COMMAND set PATH=${_qt_bin_dir}$<SEMICOLON>%PATH%
      COMMAND "${QT_WINDEPLOYQT}"
        --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
        --no-quick-import
        --no-opengl-sw
        --no-virtualkeyboard
        --no-svg
        ${WINDEPLOYQT_EXTRA_OPTIONS}
        "$<TARGET_FILE:${PROJECT_NAME}>"
      USES_TERMINAL
    )
    # copy deployment directory during installation
    install (
      DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
      DESTINATION .
    )
  endif ()
endif ()

if (APPLE AND CONFIG_INSTALL_QT)
  get_target_property (_qt_qmake_location Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
  get_filename_component (_qt_bin_dir "${_qt_qmake_location}" DIRECTORY)
  find_program(QT_MACDEPLOYQT macdeployqt HINTS "${_qt_bin_dir}")
  if (EXISTS ${QT_MACDEPLOYQT})
    add_custom_command (TARGET ${PROJECT_NAME}
      POST_BUILD
      COMMAND "${QT_MACDEPLOYQT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../.."
      USES_TERMINAL
    )
  endif ()
endif ()
