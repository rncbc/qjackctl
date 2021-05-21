cmake_minimum_required (VERSION 3.13)

project (qjackctl
  VERSION 0.9.4
  DESCRIPTION "JACK Audio Connection Kit Qt GUI Interface"
  LANGUAGES C CXX)

set (CONFIG_VERSION ${PROJECT_VERSION})
execute_process (
  COMMAND git describe --tags --dirty --abbrev=6
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
  RESULT_VARIABLE GIT_DESCRIBE_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if (GIT_DESCRIBE_RESULT EQUAL 0)
  set (VERSION "${GIT_DESCRIBE_OUTPUT}")
  string (REGEX REPLACE "^[^0-9]+" "" VERSION "${VERSION}")
  string (REGEX REPLACE "^1_"      "" VERSION "${VERSION}")
  string (REGEX REPLACE "^[_vV]+"  "" VERSION "${VERSION}")
  string (REGEX REPLACE "-g"   "git." VERSION "${VERSION}")
  string (REGEX REPLACE "[_|-]"   "." VERSION "${VERSION}")
  execute_process (
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVPARSE_OUTPUT
    RESULT_VARIABLE GIT_REVPARSE_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (GIT_REVPARSE_RESULT EQUAL 0 AND NOT GIT_REVPARSE_OUTPUT STREQUAL "master")
    set (VERSION "${VERSION} [${GIT_REVPARSE_OUTPUT}]")
  endif ()
else ()
  set (VERSION "${PROJECT_VERSION}")
endif ()

set (PACKAGE_NAME "QjackCtl")
set (PACKAGE_VERSION "${VERSION}")
set (PACKAGE_BUGREPORT "rncbc@rncbc.org")
set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set (PACKAGE_TARNAME "qjackctl")

set (ac_version "${PROJECT_VERSION}")
set (ac_prefix "${CMAKE_INSTALL_PREFIX}")

set (CONFIG_BUILD_VERSION "${PACKAGE_VERSION}")

if (CONFIG_DEBUG)
  set (CMAKE_BUILD_TYPE "Debug")
endif ()
if (CMAKE_BUILD_TYPE MATCHES "Debug")
  set (CONFIG_DEBUG 1)
endif ()
if (CONFIG_DEBUG)
  set (CONFIG_BUILD_TYPE "debug")
else ()
  set (CONFIG_BUILD_TYPE "release")
endif ()

set (CONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")

include (GNUInstallDirs)
set (CONFIG_BINDIR  "${CONFIG_PREFIX}/${CMAKE_INSTALL_BINDIR}")
set (CONFIG_LIBDIR  "${CONFIG_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set (CONFIG_DATADIR "${CONFIG_PREFIX}/${CMAKE_INSTALL_DATADIR}")
set (CONFIG_MANDIR  "${CONFIG_PREFIX}/${CMAKE_INSTALL_MANDIR}")


# Disable system tray argument option.
option (CONFIG_SYSTEM_TRAY "Enable system tray (default=yes)" 1)

# Enable JACK session support.
option (CONFIG_JACK_SESSION "Enable JACK session support (default=yes)" 1)

# Enable JACK port aliases support.
option (CONFIG_JACK_PORT_ALIASES "Enable JACK port aliases support (default=yes)" 1)

# Enable JACK metadata support.
option (CONFIG_JACK_METADATA "Enable JACK metadata support (default=yes)" 1)

# Enable JACK MIDI support option.
option (CONFIG_JACK_MIDI "Enable JACK MIDI support (default=yes)" 1)

# Enable JACK CV support option.
option (CONFIG_JACK_CV "Enable JACK CV support (default=yes)" 1)

# Enable JACK OSC support option.
option (CONFIG_JACK_OSC "Enable JACK OSC support (default=yes)" 1)

# Enable JACK version support.
option (CONFIG_JACK_VERSION "Enable JACK version support (default=no)" 0)

# Enable ALSA sequencer support option.
option (CONFIG_ALSA_SEQ "Enable ALSA/MIDI sequencer support (default=yes)" 1)

# Enable PortAudio argument option.
option (CONFIG_PORTAUDIO "Enable PortAudio interface (default=yes)" 1)

# Enable D-Bus argument option.
option (CONFIG_DBUS "Enable D-Bus interface (default=yes)" 1)

# Enable unique/single instance.
option (CONFIG_XUNIQUE "Enable unique/single instance (default=yes)" 1)

# Enable debugger stack-trace option (assumes --enable-debug).
option (CONFIG_STACKTRACE "Enable debugger stack-trace (default=no)" 0)


# Enable Qt6 build preference.
option (CONFIG_QT6 "Enable Qt6 build (default=yes)" 1)


# Fix for new CMAKE_REQUIRED_LIBRARIES policy.
if (POLICY CMP0075)
  cmake_policy (SET CMP0075 NEW)
endif ()

# Check for Qt...
if (CONFIG_QT6)
  find_package (Qt6 QUIET)
  if (NOT Qt6_FOUND)
    set (CONFIG_QT6 0)
  endif ()
endif ()

if (CONFIG_QT6)
  find_package (QT QUIET NAMES Qt6)
else ()
  find_package (QT QUIET NAMES Qt5)
endif ()

find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Xml)

if (CONFIG_XUNIQUE)
  find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
endif ()

if (CONFIG_DBUS)
  find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS DBus)
endif ()

find_package (Qt${QT_VERSION_MAJOR}LinguistTools)

include (CheckIncludeFile)
include (CheckIncludeFiles)
include (CheckIncludeFileCXX)
include (CheckFunctionExists)
include (CheckLibraryExists)

# Checks for header files.
if (UNIX AND NOT APPLE)
  check_include_files ("fcntl.h;unistd.h;signal.h" HAVE_SIGNAL_H)
endif ()


# Find package modules
find_package (PkgConfig REQUIRED)

# Check for JACK libraries.
pkg_check_modules (JACK REQUIRED IMPORTED_TARGET jack>=0.100.0)
if (JACK_FOUND)
  find_library(JACK_LIBRARY NAMES ${JACK_LIBRARIES} HINTS ${JACK_LIBDIR})
endif ()
if (JACK_LIBRARY)
  set (CONFIG_JACK 1)
  set (CMAKE_REQUIRED_LIBRARIES "${JACK_LIBRARY};${CMAKE_REQUIRED_LIBRARIES}")
  # Check for jack/statistics.h header.
  check_include_file (jack/statistics.h HAVE_JACK_STATISTICS_H)
  if (NOT HAVE_JACK_STATISTICS_H)
    set (CONFIG_JACK_STATISTICS 0)
  else ()
    set (CONFIG_JACK_STATISTICS 1)
  endif ()
  # Check for JACK MIDI headers availability.
  if (CONFIG_JACK_MIDI)
    check_include_file (jack/midiport.h HAVE_JACK_MIDIPORT_H)
    if (NOT HAVE_JACK_MIDIPORT_H)
      set (CONFIG_JACK_MIDI 0)
    endif ()
  endif ()
  # Check for JACK session headers availability.
  if (CONFIG_JACK_SESSION)
    check_include_file (jack/session.h HAVE_JACK_SESSION_H)
    if (NOT HAVE_JACK_SESSION_H)
      set (CONFIG_JACK_SESSION 0)
    endif ()
  endif ()
  # Check for JACK metadata headers availability.
  if (CONFIG_JACK_METADATA)
    check_include_file (jack/metadata.h HAVE_JACK_METADATA_H)
    if (NOT HAVE_JACK_METADATA_H)
      set (CONFIG_JACK_METADATA 0)
    endif ()
  endif ()
  if (NOT CONFIG_JACK_METADATA)
      set (CONFIG_JACK_CV  0)
      set (CONFIG_JACK_OSC 0)
  endif ()
  # Check for jack_transport_query function.
  check_function_exists (jack_transport_query CONFIG_JACK_TRANSPORT)
  # Check for jack_is_realtime function.
  check_function_exists (jack_is_realtime CONFIG_JACK_REALTIME)
  # Check for jack_get_xrun_delayed_usecs function.
  check_function_exists (jack_get_xrun_delayed_usecs CONFIG_JACK_XRUN_DELAY)
  # Check for jack_get_max_delayed_usecs function.
  check_function_exists (jack_get_max_delayed_usecs CONFIG_JACK_MAX_DELAY)
  # Check for jack_set_port_rename_callback
  check_function_exists (jack_set_port_rename_callback CONFIG_JACK_PORT_RENAME)
  # Check for jack_port_get_aliases function.
  if (CONFIG_JACK_PORT_ALIASES)
    check_function_exists (jack_port_get_aliases CONFIG_JACK_PORT_ALIASES)
  endif ()
  # Check for jack_get_version_string function.
  if (CONFIG_JACK_VERSION)
    check_function_exists (jack_get_version_string CONFIG_JACK_VERSION)
  endif ()
  # Check for jack_free function.
  check_function_exists (jack_free CONFIG_JACK_FREE)
else ()
  message (FATAL_ERROR "*** JACK library not found.")
  set (CONFIG_JACK 0)
endif ()

# Check for ALSA libraries.
if (CONFIG_ALSA_SEQ)
  pkg_check_modules (ALSA IMPORTED_TARGET alsa)
  if (ALSA_FOUND)
    find_library(ALSA_LIBRARY NAMES ${ALSA_LIBRARIES} HINTS ${ALSA_LIBDIR})
  endif ()
  if (ALSA_LIBRARY)
    set (CONFIG_ALSA_SEQ 1)
    set (CMAKE_REQUIRED_LIBRARIES "${ALSA_LIBRARY};${CMAKE_REQUIRED_LIBRARIES}")
  else ()
    message (WARNING "*** ALSA library not found.")
    set (CONFIG_ALSA_SEQ 0)
  endif ()
endif ()

# Check for PORTAUDIO libraries.
if (CONFIG_PORTAUDIO)
  pkg_check_modules (PORTAUDIO IMPORTED_TARGET portaudio-2.0)
  if (PORTAUDIO_FOUND)
    find_library(PORTAUDIO_LIBRARY NAMES ${PORTAUDIO_LIBRARIES} HINTS ${PORTAUDIO_LIBDIR})
  endif ()
  if (PORTAUDIO_LIBRARY)
    set (CONFIG_PORTAUDIO 1)
   #set (CMAKE_REQUIRED_LIBRARIES "${PORTAUDIO_LIBRARY};${CMAKE_REQUIRED_LIBRARIES}")
  else ()
    message (WARNING "*** PORTAUDIO library not found.")
    set (CONFIG_PORTAUDIO 0)
  endif ()
endif ()


if (APPLE AND CONFIG_COREAUDIO)
  check_include_file (CoreAudio/CoreAudio.h HAVE_COREAUDIO_H)
  if (NOT HAVE_COREAUDIO_H)
    set (CONFIG_COREAUDIO 0)
  endif ()
endif ()


add_subdirectory (src)

configure_file (qjackctl.spec.in qjackctl.spec IMMEDIATE @ONLY)

install (FILES qjackctl.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install (FILES qjackctl.fr.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/fr/man1 RENAME qjackctl.1)

# Configuration status
macro (SHOW_OPTION text value)
  if (${value})
    message ("${text}: yes")
  else ()
    message ("${text}: no")
  endif ()
endmacro ()


message   ("\n  ${PACKAGE_NAME} ${PACKAGE_VERSION} (Qt ${QT_VERSION})")
message   ("\n  Build target . . . . . . . . . . . . . . . . . . .: ${CONFIG_BUILD_TYPE}\n")
show_option ("  JACK Audio Connection Kit support  . . . . . . . ." CONFIG_JACK)
show_option ("  JACK Realtime support  . . . . . . . . . . . . . ." CONFIG_JACK_REALTIME)
show_option ("  JACK Transport support . . . . . . . . . . . . . ." CONFIG_JACK_TRANSPORT)
show_option ("  JACK XRUN delay support  . . . . . . . . . . . . ." CONFIG_JACK_XRUN_DELAY)
show_option ("  JACK Maximum scheduling delay support  . . . . . ." CONFIG_JACK_MAX_DELAY)
show_option ("  JACK Port aliases support  . . . . . . . . . . . ." CONFIG_JACK_PORT_ALIASES)
show_option ("  JACK Metadata support  . . . . . . . . . . . . . ." CONFIG_JACK_METADATA)
show_option ("  JACK MIDI support  . . . . . . . . . . . . . . . ." CONFIG_JACK_MIDI)
show_option ("  JACK CV support  . . . . . . . . . . . . . . . . ." CONFIG_JACK_CV)
show_option ("  JACK OSC support . . . . . . . . . . . . . . . . ." CONFIG_JACK_OSC)
show_option ("  JACK Session support . . . . . . . . . . . . . . ." CONFIG_JACK_SESSION)
show_option ("  JACK Version support (JACK2) . . . . . . . . . . ." CONFIG_JACK_VERSION)
show_option ("  ALSA MIDI Sequencer support  . . . . . . . . . . ." CONFIG_ALSA_SEQ)
show_option ("  System tray icon support . . . . . . . . . . . . ." CONFIG_SYSTEM_TRAY)
show_option ("  D-Bus interface support  . . . . . . . . . . . . ." CONFIG_DBUS)
show_option ("  PortAudio interface support  . . . . . . . . . . ." CONFIG_PORTAUDIO)
show_option ("  CoreAudio interface support  . . . . . . . . . . ." CONFIG_COREAUDIO)
message     ("")
show_option ("  System tray icon support . . . . . . . . . . . . ." CONFIG_SYSTEM_TRAY)
show_option ("  Unique/Single instance support . . . . . . . . . ." CONFIG_XUNIQUE)
show_option ("  Debugger stack-trace (gdb) . . . . . . . . . . . ." CONFIG_STACKTRACE)
message   ("\n  Install prefix . . . . . . . . . . . . . . . . . .: ${CONFIG_PREFIX}")
message   ("\nNow type 'make', followed by 'make install' as root.\n")
