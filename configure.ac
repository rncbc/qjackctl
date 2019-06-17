# Process this file with autoconf to produce a configure script.
AC_INIT(QjackCtl, 0.5.9, rncbc@rncbc.org, qjackctl)

AC_CONFIG_SRCDIR(src/qjackctl.cpp)
AC_CONFIG_HEADERS(src/config.h)
AC_CONFIG_FILES(Makefile qjackctl.spec src/src.pri)

# Build version string.
AC_CACHE_VAL([ac_cv_build_version], [
   ac_cv_build_version=$(git describe --tags --dirty --abbrev=6 2>/dev/null)
   if test -n "$ac_cv_build_version"; then
      ac_cv_build_version=$(echo $ac_cv_build_version | sed 's/^[[^_]]*[[_v]]//i')
      ac_cv_build_version=$(echo $ac_cv_build_version | sed 's/-g/git./')
      ac_cv_build_version=$(echo $ac_cv_build_version | sed 's/[[_|-]]\+/./g')
      ac_cv_build_version_extra=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
      if test "x$ac_cv_build_version_extra" != "xmaster"; then
         ac_cv_build_version="$ac_cv_build_version [[$ac_cv_build_version_extra]]"
      fi
   else
      ac_cv_build_version=$PACKAGE_VERSION
   fi
])
ac_build_version="$ac_cv_build_version"
AC_DEFINE_UNQUOTED(CONFIG_BUILD_VERSION, ["$ac_build_version"], [Build version string.])

# Sanitized version string.
AC_CACHE_VAL([ac_cv_version], [
   ac_cv_version=$(echo $PACKAGE_VERSION | sed -r 's/^([[0-9|\.]]+).*$/\1/')
])
ac_version="$ac_cv_version"
AC_DEFINE_UNQUOTED(CONFIG_VERSION, ["$ac_version"], [Version string.])
AC_SUBST(ac_version)

# Set default installation prefix.
AC_PREFIX_DEFAULT(/usr/local)
if test "x$prefix" = "xNONE"; then
   prefix=$ac_default_prefix
fi
if test "x$exec_prefix" = "xNONE"; then
   exec_prefix=$prefix
fi
eval ac_prefix=$prefix
AC_SUBST(ac_prefix)
AC_DEFINE_UNQUOTED(CONFIG_PREFIX, ["$ac_prefix"], [Default installation prefix.])

# Set default installation directories.
eval ac_bindir=$bindir
AC_SUBST(ac_bindir)
AC_DEFINE_UNQUOTED(CONFIG_BINDIR, ["$ac_bindir"], [Default executable binary path.])

eval ac_libdir=$libdir
AC_SUBST(ac_libdir)
AC_DEFINE_UNQUOTED(CONFIG_LIBDIR, ["$ac_libdir"], [Default object library path.])

eval datarootdir=$datarootdir
eval ac_datadir=$datadir
AC_SUBST(ac_datadir)
AC_DEFINE_UNQUOTED(CONFIG_DATADIR, ["$ac_datadir"], [Default arch-idependent data path.])

eval ac_mandir=$mandir
AC_SUBST(ac_mandir)
AC_DEFINE_UNQUOTED(CONFIG_MANDIR, ["$ac_mandir"], [Default man page path.])


# Enable debugging argument option.
AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--enable-debug], [enable debugging (default=no)]),
  [ac_debug="$enableval"])

if test "x$ac_debug" = "xyes"; then
   AC_DEFINE(CONFIG_DEBUG, 1, [Define if debugging is enabled.])
   ac_stacktrace="yes"
   ac_debug="debug"
else
   ac_stacktrace="no"
   ac_debug="release"
fi
AC_SUBST(ac_debug)


# Disable system tray argument option.
AC_ARG_ENABLE(system_tray,
  AS_HELP_STRING([--enable-system-tray], [enable system tray (default=yes)]),
  [ac_system_tray="$enableval"],
  [ac_system_tray="yes"])

# Disable JACK MIDI support option.
AC_ARG_ENABLE(jack_midi,
  AS_HELP_STRING([--enable-jack-midi], [enable JACK MIDI support (default=yes)]),
  [ac_jack_midi="$enableval"],
  [ac_jack_midi="yes"])

# Enable JACK session support.
AC_ARG_ENABLE(jack_session,
  AS_HELP_STRING([--enable-jack-session], [enable JACK session support (default=yes)]),
  [ac_jack_session="$enableval"],
  [ac_jack_session="yes"])

# Enable JACK port aliases support.
AC_ARG_ENABLE(jack_port_aliases,
  AS_HELP_STRING([--enable-jack-port-aliases], [enable JACK port aliases support (default=yes)]),
  [ac_jack_port_aliases="$enableval"],
  [ac_jack_port_aliases="yes"])

# Enable JACK metadata support.
AC_ARG_ENABLE(jack_metadata,
  AS_HELP_STRING([--enable-jack-metadata], [enable JACK metadata support (default=yes)]),
  [ac_jack_metadata="$enableval"],
  [ac_jack_metadata="yes"])

# Enable JACK version support.
AC_ARG_ENABLE(jack_version,
  AS_HELP_STRING([--enable-jack-version], [enable JACK version support (default=no)]),
  [ac_jack_version="$enableval"],
  [ac_jack_version="no"])

# Disable ALSA sequencer support option.
AC_ARG_ENABLE(alsa_seq,
  AS_HELP_STRING([--enable-alsa-seq], [enable ALSA/MIDI sequencer support (default=yes)]),
  [ac_alsa_seq="$enableval"],
  [ac_alsa_seq="yes"])

# Enable PortAudio argument option.
AC_ARG_ENABLE(portaudio,
  AS_HELP_STRING([--enable-portaudio], [enable PortAudio interface (default=yes)]),
  [ac_portaudio="$enableval"],
  [ac_portaudio="yes"])

# Enable D-Bus argument option.
AC_ARG_ENABLE(dbus,
  AS_HELP_STRING([--enable-dbus], [enable D-Bus interface (default=yes)]),
  [ac_dbus="$enableval"],
  [ac_dbus="yes"])

# Enable unique/single instance.
AC_ARG_ENABLE(xunique,
  AS_HELP_STRING([--enable-xunique], [enable unique/single instance (default=yes)]),
  [ac_xunique="$enableval"],
  [ac_xunique="yes"])

# Enable debugger stack-trace option (assumes --enable-debug).
AC_ARG_ENABLE(stacktrace,
  AS_HELP_STRING([--enable-stacktrace], [enable debugger stack-trace (default=no)]),
  [ac_stacktrace="$enableval"])


# Standard installation base dirs.
ac_path=$PATH
ac_with_paths=""

# Set for alternate Qt installation dir.
AC_ARG_WITH(qt,
  AS_HELP_STRING([--with-qt=PATH], [use alternate Qt install path]),
  [ac_qt_path="$withval"], [ac_qt_path="no"])

if test "x$ac_qt_path" != "xno"; then
   ac_path="$ac_qt_path/bin:$ac_path"
fi

# Set for alternate JACK installation dir.
AC_ARG_WITH(jack,
  AS_HELP_STRING([--with-jack=PATH], [use alternate JACK install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Set for alternate ALSA installation dir.
AC_ARG_WITH(alsa,
  AS_HELP_STRING([--with-alsa=PATH], [use alternate ALSA install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Set for alternate PortAudio installation dir.
AC_ARG_WITH(portaudio,
  AS_HELP_STRING([--with-portaudio=PATH], [use alternate PortAudio install path]),
  [ac_with_paths="$ac_with_paths $withval"])


# Honor user specified flags.
ac_cflags=$CFLAGS
ac_ldflags=$LDFLAGS


# Checks for programs.
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP

# Checks for languages.
AC_LANG_CPLUSPLUS

# Check for pkg-config.
PKG_PROG_PKG_CONFIG

# Check whether -std=c++11 support is necessary (4 < g++ version < 6).
AC_CACHE_CHECK([for g++ major version], [ac_cv_gxx_version_major], [
   ac_cv_gxx_version_major=$($CC -dumpversion)
   if test -n "$ac_cv_gxx_version_major"; then
      ac_cv_gxx_version_major=$(echo $ac_cv_gxx_version_major | cut -d'.' -f1);
   fi
   ac_gxx_version_major=$(($ac_cv_gxx_version_major + 0))
])
ac_gxx_version_major=$ac_cv_gxx_version_major
if test $ac_gxx_version_major -ge 4 -a $ac_gxx_version_major -lt 6; then
   CXXFLAGS="-std=c++11 $CXXFLAGS"
fi

# Check for proper flags.
ac_arch=$(uname -m)

# Check for install paths and alternatives...
ac_libdirs="lib"

if test "x$ac_arch" = "xx86_64"; then
   ac_libdirs="$ac_libdirs lib64"
fi

CFLAGS="-fPIC $CFLAGS"
CPPFLAGS="-fPIC $CPPFLAGS"

# Prepend alternate dependencies paths.
for X in $ac_with_paths; do
  if test -d $X/bin; then
     ac_path="$X/bin:$ac_path"
  fi
  if test -d $X/include; then
     CFLAGS="-I$X/include $CFLAGS "
     CPPFLAGS="-I$X/include $CPPFLAGS"
     ac_incpath="$X/include $ac_incpath"
  fi
  for Y in $ac_libdirs; do
     if test -d $X/$Y; then
        LIBS="-L$X/$Y $LIBS"
        ac_libs="-L$X/$Y $ac_libs"
     fi
  done
done

# A common error message:
ac_errmsg="not found in current PATH. Maybe QT development environment isn't available."

# Check for qtchooser availability.
AC_PATH_TOOL(ac_qtchooser, qtchooser, [no], $ac_path)
if test -x $ac_qtchooser; then
   export QT_SELECT=5
fi

# Check for proper qmake path/version alternatives.
AC_PATH_TOOL(ac_qmake, qmake, [no], $ac_path)
if test "x$ac_qmake" = "xno"; then
   AC_PATH_TOOL(ac_cv_qmake, qmake-qt5, [no], $ac_path)
   ac_qmake=$ac_cv_qmake
fi

# Check for proper Qt major version.
AC_CACHE_CHECK([for Qt major version], [ac_cv_qt_version_major], [
   ac_cv_qt_version_major=$($ac_qmake -query QT_VERSION | cut -d'.' -f1)
   ac_cv_qt_version_major=$(($ac_cv_qt_version_major + 0))
])
ac_qt_version_major=$ac_cv_qt_version_major
if test $ac_qt_version_major -ne 5; then
	AC_PATH_TOOL(ac_cv_qmake, qmake-qt5, [no], $ac_path)
	ac_qmake=$ac_cv_qmake
fi

if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake-qt5 $ac_errmsg (qt5-devel)])
fi

# Check for proper Qt install path.
AC_CACHE_CHECK([for Qt install path], [ac_cv_qt_install_path], [
   ac_cv_qt_install_path=$($ac_qmake -query QT_INSTALL_BINS)
])
ac_qt_install_path=$ac_cv_qt_install_path
if test -d $ac_qt_install_path; then
   ac_path="$ac_qt_install_path:$ac_path"
fi

# Check it again, now with updated PATH, just in case...
AC_PATH_TOOL(ac_cv_qmake, qmake, [no], $ac_path)
ac_qmake=$ac_cv_qmake
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_errmsg])
fi

AC_SUBST(ac_qmake)

AC_CACHE_CHECK([for Qt install headers], [ac_cv_qt_install_headers], [
   ac_cv_qt_install_headers=$($ac_qmake -query QT_INSTALL_HEADERS)
])
ac_qt_install_headers=$ac_cv_qt_install_headers
if test -d $ac_qt_install_headers; then
   CFLAGS="-I$ac_qt_install_headers $CFLAGS "
   CPPFLAGS="-I$ac_qt_install_headers $CPPFLAGS"
   ac_incpath="$ac_qt_install_headers $ac_incpath"
fi

AC_CACHE_CHECK([for Qt install libraries], [ac_cv_qt_install_libs], [
   ac_cv_qt_install_libs=$($ac_qmake -query QT_INSTALL_LIBS)
])
ac_qt_install_libs=$ac_cv_qt_install_libs
if test -d $ac_qt_install_libs; then
   LIBS="-L$ac_qt_install_libs $LIBS"
   ac_libs="-L$ac_qt_install_libs $ac_libs"
fi

# Finally, check for proper Qt version.
AC_CACHE_CHECK([for Qt library version >= 5.1],
   ac_cv_qtversion, [
   AC_TRY_COMPILE([#include "QtCore/qglobal.h"], [
      #if QT_VERSION < 0x050100
      #error Qt library 5.1 or greater required.
      #endif
   ], ac_cv_qtversion="yes", [
      echo "no; Qt 5.1 or greater is required"
      exit 1
   ])
])

# Check for Qt moc utility.
AC_PATH_TOOL(ac_moc, moc, [no], $ac_path)
if test "x$ac_moc" = "xno"; then
   AC_MSG_ERROR([moc $ac_errmsg])
fi
AC_SUBST(ac_moc)

# Check for Qt uic utility.
AC_PATH_TOOL(ac_uic, uic, [no], $ac_path)
if test "x$ac_uic" = "xno"; then
   AC_MSG_ERROR([uic $ac_errmsg])
fi
AC_SUBST(ac_uic)

# Check for Qt lupdate utility.
AC_PATH_TOOL(ac_lupdate, lupdate, [no], $ac_path)
if test "x$ac_lupdate" = "xno"; then
   AC_MSG_ERROR([lupdate $ac_errmsg])
fi
AC_SUBST(ac_lupdate)

# Check for Qt lrelease utility.
AC_PATH_TOOL(ac_lrelease, lrelease, [no], $ac_path)
if test "x$ac_release" = "xno"; then
   AC_MSG_ERROR([lrelease $ac_errmsg])
fi
AC_SUBST(ac_lrelease)


# Checks for libraries.
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(X11, main)
AC_CHECK_LIB(Xext, main)

# Check for round math function.
AC_CHECK_LIB(m, lroundf, [ac_round="yes"], [ac_round="no"])
if test "x$ac_round" = "xyes"; then
   AC_DEFINE(CONFIG_ROUND, 1, [Define if round is available.])
fi

# Check for JACK libraries.
PKG_CHECK_MODULES([JACK], [jack >= 0.100.0], [ac_jack_lib="yes"], [ac_jack_lib="no"])
if test "x$ac_jack_lib" = "xyes"; then
   AC_DEFINE(CONFIG_JACK, 1, [Define if JACK library is available.])
   ac_cflags="$ac_cflags $JACK_CFLAGS"
   ac_libs="$ac_libs $JACK_LIBS"
   CFLAGS="$CFLAGS $JACK_CFLAGS"
   CPPFLAGS="$CPPFLAGS $JACK_CFLAGS"
   LIBS="$LIBS $JACK_LIBS"
else
   AC_MSG_ERROR([*** JACK library not found.])
fi

# Check for ALSA libraries and primarily
# for ALSA/MIDI sequencer support.
if test "x$ac_alsa_seq" = "xyes"; then
   PKG_CHECK_MODULES([ALSA], [alsa], [ac_alsa_seq="yes"], [ac_alsa_seq="no"])
fi
if test "x$ac_alsa_seq" = "xyes"; then
   AC_DEFINE(CONFIG_ALSA_SEQ, 1, [Define if ALSA library is available.])
   ac_cflags="$ac_cflags $ALSA_CFLAGS"
   ac_libs="$ac_libs $ALSA_LIBS"
else
   AC_MSG_WARN([*** ALSA library not found.])
fi

if test "x$ac_portaudio" = "xyes"; then
   PKG_CHECK_MODULES([PORTAUDIO], [portaudio-2.0], [ac_portaudio="yes"], [ac_portaudio="no"])
fi
if test "x$ac_portaudio" = "xyes"; then
   AC_DEFINE(CONFIG_PORTAUDIO 1, [Define if PORTAUDIO library is available.])
   ac_cflags="$ac_cflags $PORTAUDIO_CFLAGS"
   ac_libs="$ac_libs $PORTAUDIO_LIBS"
else
   AC_MSG_WARN([*** PORTAUDIO library not found.])
fi


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h poll.h signal.h)

# Check for jack/statistics.h header.
AC_CHECK_HEADER(jack/statistics.h, [ac_jack_statistics_h="yes"], [ac_jack_statistics_h="no"])
if test "x$ac_jack_statistics_h" = "xyes"; then
   AC_DEFINE(CONFIG_JACK_STATISTICS, 1, [Define if jack/statistics.h is available.])
fi

# Check for CoreAudio/CoreAudio.h header (MacOSX).
AC_CHECK_HEADER(CoreAudio/CoreAudio.h, [ac_coreaudio="yes"], [ac_coreaudio="no"])
if test "x$ac_coreaudio" = "xyes"; then
   AC_DEFINE(CONFIG_COREAUDIO, 1, [Define if CoreAudio/CoreAudio.h is available (Mac OS X).])
   ac_alsa_seq="no"
   ac_libs="$ac_libs -framework CoreAudio -framework CoreFoundation"
fi

# Check for JACK MIDI headers availability.
if test "x$ac_jack_midi" = "xyes"; then
   AC_CHECK_HEADER(jack/midiport.h, [ac_jack_midi="yes"], [ac_jack_midi="no"])
   if test "x$ac_jack_midi" = "xyes"; then
      AC_DEFINE(CONFIG_JACK_MIDI, 1, [Define if JACK MIDI support is available.])
   else
      AC_MSG_WARN([*** jack/midiport.h file not found.])
      AC_MSG_WARN([*** JACK 0.103.0 or later may be required.])
      AC_MSG_WARN([*** JACK MIDI support will be disabled.])
   fi
fi

# Check for JACK session headers availability.
if test "x$ac_jack_session" = "xyes"; then
   AC_CHECK_HEADER(jack/session.h, [ac_jack_session="yes"], [ac_jack_session="no"])
   if test "x$ac_jack_session" = "xyes"; then
      AC_DEFINE(CONFIG_JACK_SESSION, 1, [Define if JACK session support is available.])
   else
      AC_MSG_WARN([*** jack/session.h file not found.])
      AC_MSG_WARN([*** JACK 0.118.3 or later may be required.])
      AC_MSG_WARN([*** JACK session support will be disabled.])
   fi
fi

# Check for JACK metadata headers availability.
if test "x$ac_jack_metadata" = "xyes"; then
   AC_CHECK_HEADER(jack/metadata.h, [ac_jack_metadata="yes"], [ac_jack_metadata="no"])
   if test "x$ac_jack_metadata" = "xyes"; then
      AC_DEFINE(CONFIG_JACK_METADATA, 1, [Define if JACK metadata support is available.])
   else
      AC_MSG_WARN([*** jack/metadata.h file not found.])
      AC_MSG_WARN([*** JACK 0.124.1 or later may be required.])
      AC_MSG_WARN([*** JACK metadata support will be disabled.])
   fi
fi

# Check D-Bus interface support.
if test "x$ac_dbus" = "xyes"; then
   PKG_CHECK_MODULES([QT5DBUS], [Qt5DBus], [ac_dbus="yes"], [ac_dbus="no"])
fi
if test "x$ac_dbus" = "xyes"; then
   AC_DEFINE(CONFIG_DBUS, 1, [Define if D-Bus interface is enabled.])
   ac_qdbus="dbus"
fi
AC_SUBST(ac_qdbus)

# Check for unique/single instance support.
if test "x$ac_xunique" = "xyes"; then
   PKG_CHECK_MODULES([QT5NETWORK], [Qt5Network], [ac_qt5network="yes"], [ac_qt5network="no"])
   if test "x$ac_qt5network" = "xyes"; then
      AC_DEFINE(CONFIG_XUNIQUE, 1, [Define if unique/single instance is enabled.])
      ac_qnetwork="network"
   fi
fi
AC_SUBST(ac_qnetwork)

# Check for debugging stack-trace.
if test "x$ac_stacktrace" = "xyes"; then
   AC_DEFINE(CONFIG_STACKTRACE, 1, [Define if debugger stack-trace is enabled.])
fi

AC_SUBST(ac_cflags)
AC_SUBST(ac_ldflags)
AC_SUBST(ac_incpath)
AC_SUBST(ac_libs)


# Checks for typedefs, structures, and compiler characteristics.
# AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS(system)

# Check for system-tray icon.
if test "x$ac_system_tray" = "xyes"; then
   AC_DEFINE(CONFIG_SYSTEM_TRAY, 1, [Define if system tray is enabled.])
fi

# Check for jack_transport_query function.
AC_CHECK_LIB(jack, jack_transport_query, [ac_jack_transport="yes"], [ac_jack_transport="no"])
if test "x$ac_jack_transport" = "xno"; then
   AC_MSG_WARN([*** jack_transport_query: function not found.])
   AC_MSG_WARN([*** JACK 0.80.0 or later may be required.])
   AC_MSG_WARN([*** Transport features will be disabled.])
else
   AC_DEFINE(CONFIG_JACK_TRANSPORT, 1, [Define if jack_tranport_query is available.])
fi

# Check for jack_is_realtime function.
AC_CHECK_LIB(jack, jack_is_realtime, [ac_jack_realtime="yes"], [ac_jack_realtime="no"])
if test "x$ac_jack_realtime" = "xyes"; then
   AC_DEFINE(CONFIG_JACK_REALTIME, 1, [Define if jack_is_realtime is available.])
fi

# Check for jack_get_xrun_delayed_usecs function.
AC_CHECK_LIB(jack, jack_get_xrun_delayed_usecs, [ac_jack_xrun_delay="yes"], [ac_jack_xrun_delay="no"])
if test "x$ac_jack_xrun_delay" = "xyes"; then
   AC_DEFINE(CONFIG_JACK_XRUN_DELAY, 1, [Define if jack_get_xrun_delayed_usecs is available.])
fi

# Check for jack_get_max_delayed_usecs function.
AC_CHECK_LIB(jack, jack_get_max_delayed_usecs, [ac_jack_max_delay="yes"], [ac_jack_max_delay="no"])
if test "x$ac_jack_max_delay" = "xyes"; then
   AC_DEFINE(CONFIG_JACK_MAX_DELAY, 1, [Define if jack_get_max_delayed_usecs is available.])
fi

# Check for jack_set_port_rename_callback
AC_CHECK_LIB(jack, jack_set_port_rename_callback, [ac_jack_port_rename="yes"], [ac_jack_port_rename="no"])
if test "x$ac_jack_port_rename" = "xyes"; then
   AC_TRY_COMPILE([#include <jack/jack.h>], [
      void port_rename(jack_port_id_t, const char *, const char *, void *);
      jack_set_port_rename_callback(0, port_rename, 0);
   ], ac_jack_port_rename="yes", ac_jack_port_rename="no")
fi
if test "x$ac_jack_port_rename" = "xyes"; then
   AC_DEFINE(CONFIG_JACK_PORT_RENAME, 1, [Define if jack_set_port_rename_callback is available.])
fi

# Check for jack_port_get_aliases function.
if test "x$ac_jack_port_aliases" = "xyes"; then
   AC_CHECK_LIB(jack, jack_port_get_aliases, [ac_jack_port_aliases="yes"], [ac_jack_port_aliases="no"])
   if test "x$ac_jack_port_aliases" = "xyes"; then
      AC_DEFINE(CONFIG_JACK_PORT_ALIASES, 1, [Define if jack_port_get_aliases is available.])
   fi
fi

# Check for jack_get_version_string function.
if test "x$ac_jack_version" = "xyes"; then
   AC_CHECK_LIB(jack, jack_get_version_string, [ac_jack_version="yes"], [ac_jack_version="no"])
   if test "x$ac_jack_version" = "xyes"; then
      AC_DEFINE(CONFIG_JACK_VERSION, 1, [Define if jack_get_version_string is available.])
   fi
fi

# Check for jack_free function.
AC_CHECK_LIB(jack, jack_free, [ac_jack_free="yes"], [ac_jack_free="no"])
if test "x$ac_jack_free" = "xyes"; then
   AC_DEFINE(CONFIG_JACK_FREE, 1, [Define if jack_free is available.])
fi

# Finally produce a configure header file and the main makefile.
AC_OUTPUT

# make clean > /dev/null 2>&1

# Output summary message

echo
echo "  $PACKAGE_NAME $ac_build_version"
echo
echo "  Build target . . . . . . . . . . . . . . . . . . .: $ac_debug"
echo
echo "  JACK Audio Connection Kit support  . . . . . . . .: $ac_jack_lib"
echo "  JACK Realtime support  . . . . . . . . . . . . . .: $ac_jack_realtime"
echo "  JACK Transport support . . . . . . . . . . . . . .: $ac_jack_transport"
echo "  JACK XRUN delay support  . . . . . . . . . . . . .: $ac_jack_xrun_delay"
echo "  JACK Maximum scheduling delay support  . . . . . .: $ac_jack_max_delay"
echo "  JACK Port aliases support  . . . . . . . . . . . .: $ac_jack_port_aliases"
echo "  JACK Metadata support  . . . . . . . . . . . . . .: $ac_jack_metadata"
echo "  JACK MIDI support  . . . . . . . . . . . . . . . .: $ac_jack_midi"
echo "  JACK Session support . . . . . . . . . . . . . . .: $ac_jack_session"
echo "  JACK Version support (JACK2) . . . . . . . . . . .: $ac_jack_version"
echo "  ALSA MIDI Sequencer support  . . . . . . . . . . .: $ac_alsa_seq"
echo "  System tray icon support . . . . . . . . . . . . .: $ac_system_tray"
echo "  D-Bus interface support  . . . . . . . . . . . . .: $ac_dbus"
echo "  PortAudio interface support  . . . . . . . . . . .: $ac_portaudio"
echo "  CoreAudio interface support  . . . . . . . . . . .: $ac_coreaudio"
echo
echo "  Unique/Single instance support . . . . . . . . . .: $ac_xunique"
echo "  Debugger stack-trace (gdb) . . . . . . . . . . . .: $ac_stacktrace"
echo
echo "  Install prefix . . . . . . . . . . . . . . . . . .: $ac_prefix"
echo
echo "Now type 'make', followed by 'make install' as root."
echo

