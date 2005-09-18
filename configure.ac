# Process this file with autoconf to produce a configure script.
AC_INIT(QjackCtl, 0.2.18.11, rncbc@rncbc.org, qjackctl)

AC_CONFIG_SRCDIR(src/qjackctlMainForm.ui.h)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(Makefile qjackctl.pro qjackctl.spec)

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
  [ac_debug="debug"],
  [ac_debug="release"])
AC_SUBST(ac_debug)

if test "x$ac_debug" = "xdebug"; then
   AC_DEFINE(CONFIG_DEBUG, 1, [Define if debugging is enabled.])
fi

# Disable system tray argument option.
AC_ARG_ENABLE(system_tray,
  AC_HELP_STRING([--disable-system-tray], [disable system tray (default=no)]),
  [ac_system_tray="no"],
  [ac_system_tray="yes"])

if test "x$ac_system_tray" = "xyes"; then
   AC_DEFINE(CONFIG_SYSTEM_TRAY, 1, [Define if system tray is enabled.])
fi

# Disable ALSA sequencer support option.
AC_ARG_ENABLE(alsa_seq,
  AC_HELP_STRING([--disable-alsa-seq], [disable ALSA/MIDI sequencer support (default=no)]),
  [ac_alsa_seq="no"],
  [ac_alsa_seq="yes"])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_GCC_TRADITIONAL

# Checks for languages.
AC_LANG_C
AC_LANG_CPLUSPLUS

# Check for QTDIR environment variable.
AC_MSG_CHECKING([whether QTDIR environment variable is set])
if test "x$QTDIR" = "x"; then
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([QTDIR must be properly set.])
else
  AC_MSG_RESULT([$QTDIR])
fi

CFLAGS="$CFLAGS -I$QTDIR/include"
CPPFLAGS="$CPPFLAGS -I$QTDIR/include"
LIBS="-L$QTDIR/lib"
for X in qt qt3; do
  if test -d $QTDIR/include/$X; then
    CFLAGS="$CFLAGS -I$QTDIR/include/$X"
    CPPFLAGS="$CPPFLAGS -I$QTDIR/include/$X"
  fi
done

# Checks for Qt library.
for X in qt-mt qt; do
  if test "x$ac_qtlib" = "x"; then
     AC_CHECK_LIB($X, main, [ac_qtlib=$X])
  fi
done
if test "x$ac_qtlib" = "x"; then
  AC_MSG_ERROR([Qt library not found. Maybe QTDIR isn't properly set.])
fi
AC_SUBST(ac_qtlib)

# Check for Qt multi-thread support.
if test "x$ac_qtlib" = "xqt-mt"; then
  ac_thread="thread"
fi
AC_SUBST(ac_thread)

AC_CACHE_CHECK([for Qt library version >= 3.1.1],
  ac_qtlib_version, [
  AC_TRY_LINK([#include "qglobal.h"], [
#if QT_VERSION < 0x030101
#error Qt library 3.1.1 or greater required.
#endif
    ],
    ac_qtlib_version="yes", [
      echo "no; Qt 3.1.1 or greater is required"
      exit
    ])
])

# A common error message:
ac_qtdir_errmsg="not found in current PATH. Maybe QT development environment isn't available (qt3-devel)."

# Check for Qt qmake utility.
AC_PATH_PROG(ac_qmake, qmake, [no], $QTDIR/bin:${PATH})
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_qtdir_errmsg])
fi
AC_SUBST(ac_qmake)

# Check for Qt moc utility.
AC_PATH_PROG(ac_moc, moc, [no], $QTDIR/bin:${PATH})
if test "x$ac_moc" = "xno"; then
   AC_MSG_ERROR([moc $ac_qtdir_errmsg])
fi
AC_SUBST(ac_moc)

# Check for Qt uic utility.
AC_PATH_PROG(ac_uic, uic, [no], $QTDIR/bin:${PATH})
if test "x$ac_uic" = "xno"; then
   AC_MSG_ERROR([uic $ac_qtdir_errmsg])
fi
AC_SUBST(ac_uic)

# Checks for libraries.
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(X11, main)
AC_CHECK_LIB(Xext, main)
AC_CHECK_LIB($ac_qtlib, main)

# Check for JACK libraries.
AC_CHECK_LIB(jack, main, [ac_jack_lib="yes"], [ac_jack_lib="no"])
if test "x$ac_jack_lib" = "xno"; then
   AC_MSG_ERROR([JACK library not found.])
fi
ac_libs="-ljack"

# Check for ALSA libraries and primarily
# for ALSA/MIDI sequencer support.
if test "x$ac_alsa_seq" = "xyes"; then
   AC_CHECK_LIB(asound, main, [ac_alsa_lib="yes"], [ac_alsa_lib="no"])
   if test "x$ac_alsa_lib" = "xno"; then
      AC_MSG_WARN([*** ALSA library not found.])
      ac_alsa_seq="no"
   else
      ac_libs="-lasound $ac_libs"
   fi
fi

AC_SUBST(ac_libs)

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

# Check for CoreAudio/CoreAudio.h header (MacOSX).
AC_CHECK_HEADER(CoreAudio/CoreAudio.h, [ac_coreaudio="yes"], [ac_coreaudio="no"])
if test "x$ac_coreaudio" = "xyes"; then
   AC_DEFINE(CONFIG_COREAUDIO, 1, [Define if CoreAudio/CoreAudio.h is available (Mac OS X).])
   ac_libs="-framework CoreAudio $ac_libs" 
fi

# Checks for typedefs, structures, and compiler characteristics.
# AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS(system)

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

# Finally produce a configure header file and the main makefile.
AC_OUTPUT

make clean > /dev/null 2>&1

echo ""
echo "Now type 'make', followed by 'make install' as root."
echo ""

