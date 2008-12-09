# Process this file with autoconf to produce a configure script.
AC_INIT(QjackCtl, 0.3.4.2, rncbc@rncbc.org, qjackctl)

AC_CONFIG_SRCDIR(src/qjackctlMainForm.ui)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(Makefile qjackctl.pro qjackctl.spec qjackctl.desktop)

# Set default installation prefix.
AC_PREFIX_DEFAULT(/usr/local)
ac_prefix=$prefix
if test "x$ac_prefix" = "xNONE"; then
    ac_prefix=$ac_default_prefix
fi
AC_SUBST(ac_prefix)
AC_DEFINE_UNQUOTED(CONFIG_PREFIX, ["$ac_prefix"], [Default installation prefix.])

# Enable debugging argument option.
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug], [enable debugging (default=no)]),
  [ac_debug="$enableval"])

if test "x$ac_debug" = "xyes"; then
   AC_DEFINE(CONFIG_DEBUG, 1, [Define if debugging is enabled.])
   ac_debug="debug"
else
   ac_debug="release"
fi
AC_SUBST(ac_debug)

# Disable system tray argument option.
AC_ARG_ENABLE(system_tray,
  AC_HELP_STRING([--enable-system-tray], [enable system tray (default=yes)]),
  [ac_system_tray="$enableval"],
  [ac_system_tray="yes"])

# Disable JACK MIDI support option.
AC_ARG_ENABLE(jack_midi,
  AC_HELP_STRING([--enable-jack-midi], [enable JACK MIDI support (default=yes)]),
  [ac_jack_midi="$enableval"],
  [ac_jack_midi="yes"])

# Disable ALSA sequencer support option.
AC_ARG_ENABLE(alsa_seq,
  AC_HELP_STRING([--enable-alsa-seq], [enable ALSA/MIDI sequencer support (default=yes)]),
  [ac_alsa_seq="$enableval"],
  [ac_alsa_seq="yes"])

# Standard installation base dirs.
ac_with_paths="/usr /usr/local"

# Some a-la-debian alternatives...
for X in /usr/lib /usr/share; do
  for Y in qt qt4; do
    if test -d $X/$Y/bin; then
      ac_with_paths="$ac_with_paths $X/$Y"
    fi
  done
done

# Set for alternate Qt installation dir.
AC_ARG_WITH(qt,
  AC_HELP_STRING([--with-qt=PATH], [use alternate Qt install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Set for alternate JACK installation dir.
AC_ARG_WITH(jack,
  AC_HELP_STRING([--with-jack=PATH], [use alternate JACK install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Set for alternate ALSA installation dir.
AC_ARG_WITH(alsa,
  AC_HELP_STRING([--with-alsa=PATH], [use alternate ALSA install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_GCC_TRADITIONAL

# Checks for languages.
AC_LANG_C
AC_LANG_CPLUSPLUS

# Prepend alternate dependencies paths.
ac_path=$PATH
for X in $ac_with_paths; do
  if test -d $X/bin; then
    ac_path="$X/bin:$ac_path"
  fi
  if test -x $X/qmake; then
    ac_path="$X:$ac_path"
  fi
  if test -d $X/include; then
    for Y in qt qt4; do
      if test -d $X/include/$Y; then
        CFLAGS="-I$X/include/$Y $CFLAGS"
        CPPFLAGS="-I$X/include/$Y $CPPFLAGS"
        ac_incpath="$X/include/$Y $ac_incpath"
      fi
    done
    CFLAGS="-I$X/include $CFLAGS"
    CPPFLAGS="-I$X/include $CPPFLAGS"
    ac_incpath="$X/include $ac_incpath"
  fi
  if test -d $X/lib64; then
    LIBS="-L$X/lib64 $LIBS"
    ac_libs="-L$X/lib64 $ac_libs"
  fi
  if test -d $X/lib; then
    LIBS="-L$X/lib $LIBS"
    ac_libs="-L$X/lib $ac_libs"
  fi
done

# Check for proper Qt version.
AC_CACHE_CHECK([for Qt library version >= 4.1],
  ac_qtlib_version, [
  AC_TRY_LINK([#include "Qt/qglobal.h"], [
#if QT_VERSION < 0x040100
#error Qt library 4.1 or greater required.
#endif
    ],
    ac_qtlib_version="yes", [
      echo "no; Qt 4.1 or greater is required"
      exit
    ])
])

# A common error message:
ac_errmsg="not found in current PATH. Maybe QT development environment isn't available (qt-devel)."

# Check for Qt qmake utility.
AC_PATH_PROG(ac_qmake, qmake, [no], $ac_path)
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_errmsg])
fi
AC_SUBST(ac_qmake)

# Check for Qt moc utility.
AC_PATH_PROG(ac_moc, moc, [no], $ac_path)
if test "x$ac_moc" = "xno"; then
   AC_MSG_ERROR([moc $ac_errmsg])
fi
AC_SUBST(ac_moc)

# Check for Qt uic utility.
AC_PATH_PROG(ac_uic, uic, [no], $ac_path)
if test "x$ac_uic" = "xno"; then
   AC_MSG_ERROR([uic $ac_errmsg])
fi
AC_SUBST(ac_uic)

# Check for Qt lupdate utility.
AC_PATH_PROG(ac_lupdate, lupdate, [no], $ac_path)
if test "x$ac_lupdate" = "xno"; then
   AC_MSG_ERROR([lupdate $ac_errmsg])
fi
AC_SUBST(ac_lupdate)

# Check for Qt lrelease utility.
AC_PATH_PROG(ac_lrelease, lrelease, [no], $ac_path)
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
AC_CHECK_LIB(jack, main, [ac_jack_lib="yes"], [ac_jack_lib="no"])
if test "x$ac_jack_lib" = "xno"; then
   AC_MSG_ERROR([JACK library not found.])
fi
ac_libs="$ac_libs -ljack"

# Check for ALSA libraries and primarily
# for ALSA/MIDI sequencer support.
if test "x$ac_alsa_seq" = "xyes"; then
   AC_CHECK_LIB(asound, main, [ac_alsa_lib="yes"], [ac_alsa_lib="no"])
   if test "x$ac_alsa_lib" = "xno"; then
      AC_MSG_WARN([*** ALSA library not found.])
      ac_alsa_seq="no"
   else
      ac_libs="$ac_libs -lasound"
   fi
fi

AC_SUBST(ac_libs)
AC_SUBST(ac_incpath)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h poll.h signal.h)

# Check for JACK headers.
AC_CHECK_HEADER(jack/jack.h, [ac_jack_h="yes"], [ac_jack_h="no"])
if test "x$ac_jack_h" = "xno"; then
   AC_MSG_ERROR([JACK headers not found.])
fi

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
   ac_libs="-framework CoreAudio -framework CoreFoundation $ac_libs"
fi

# Check for some JACK MIDI headers.
if test "x$ac_jack_midi" = "xyes"; then
   AC_CHECK_HEADER(jack/midiport.h, [ac_jack_midi="yes"], [ac_jack_midi="no"])
fi

# Check for ALSA headers and ultimately
# for ALSA/MIDI sequencer support.
if test "x$ac_alsa_seq" = "xyes"; then
   AC_CHECK_HEADER(alsa/asoundlib.h, [ac_alsa_h="yes"], [ac_alsa_h="no"])
   if test "x$ac_alsa_h" = "xno"; then
      AC_MSG_ERROR([ALSA headers not found.])
   fi
   AC_DEFINE(CONFIG_ALSA_SEQ, 1, [Define if ALSA/MIDI sequencer support is enabled.])
else
   AC_MSG_WARN([*** ALSA/MIDI sequencer support will be disabled.])
fi

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

# Check for JACK MIDI library support.
if test "x$ac_jack_midi" = "xyes"; then
   AC_CHECK_LIB(jack, jack_midi_event_get, [ac_jack_midi="yes"], [ac_jack_midi="no"])
   if test "x$ac_jack_midi" = "xno"; then
      AC_MSG_WARN([*** jack_midi_event_get: function not found.])
      AC_MSG_WARN([*** JACK 0.103.0 or later may be required.])
      AC_MSG_WARN([*** JACK MIDI support will be disabled.])
   else
      AC_DEFINE(CONFIG_JACK_MIDI, 1, [Define if JACK MIDI support is available.])
   fi
fi

# Check for jack_port_get_aliases function.
AC_CHECK_LIB(jack, jack_port_get_aliases, [ac_jack_port_aliases="yes"], [ac_jack_port_aliases="no"])
if test "x$ac_jack_port_aliases" = "xyes"; then
   AC_DEFINE(CONFIG_JACK_PORT_ALIASES, 1, [Define if jack_port_get_aliases is available.])
fi

# Finally produce a configure header file and the main makefile.
AC_OUTPUT

# make clean > /dev/null 2>&1

# Output summary message

echo
echo "  $PACKAGE_NAME $PACKAGE_VERSION"
echo
echo "  Build target . . . . . . . . . . . . . . . . . . .: $ac_debug"
echo
echo "  JACK Audio Connection Kit support  . . . . . . . .: $ac_jack_lib"
echo "  JACK Realtime support  . . . . . . . . . . . . . .: $ac_jack_realtime"
echo "  JACK Transport support . . . . . . . . . . . . . .: $ac_jack_transport"
echo "  JACK XRUN delay support  . . . . . . . . . . . . .: $ac_jack_xrun_delay"
echo "  JACK Maximum scheduling delay support  . . . . . .: $ac_jack_max_delay"
echo "  JACK Port aliases support  . . . . . . . . . . . .: $ac_jack_port_aliases"
echo "  JACK MIDI support  . . . . . . . . . . . . . . . .: $ac_jack_midi"
echo "  ALSA MIDI Sequencer support  . . . . . . . . . . .: $ac_alsa_seq"
echo "  System tray icon support . . . . . . . . . . . . .: $ac_system_tray"
echo
echo "  Install prefix . . . . . . . . . . . . . . . . . .: $ac_prefix"
echo
echo "Now type 'make', followed by 'make install' as root."
echo
