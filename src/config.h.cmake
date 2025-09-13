#ifndef CONFIG_H
#define CONFIG_H

/* Define to the title of this package. */
#cmakedefine PROJECT_TITLE "@PROJECT_TITLE@"

/* Define to the name of this package. */
#cmakedefine PROJECT_NAME "@PROJECT_NAME@"

/* Define to the version of this package. */
#cmakedefine PROJECT_VERSION "@PROJECT_VERSION@"

/* Define to the description of this package. */
#cmakedefine PROJECT_DESCRIPTION "@PROJECT_DESCRIPTION@"

/* Define to the homepage of this package. */
#cmakedefine PROJECT_HOMEPAGE_URL "@PROJECT_HOMEPAGE_URL@"

/* Define to the copyright of this package. */
#cmakedefine PROJECT_COPYRIGHT "@PROJECT_COPYRIGHT@"

/* Define to the domain of this package. */
#cmakedefine PROJECT_DOMAIN "@PROJECT_DOMAIN@"


/* Default installation prefix. */
#cmakedefine CONFIG_PREFIX "@CONFIG_PREFIX@"

/* Define to target installation dirs. */
#cmakedefine CONFIG_BINDIR "@CONFIG_BINDIR@"
#cmakedefine CONFIG_LIBDIR "@CONFIG_LIBDIR@"
#cmakedefine CONFIG_DATADIR "@CONFIG_DATADIR@"
#cmakedefine CONFIG_MANDIR "@CONFIG_MANDIR@"

/* Define if debugging is enabled. */
#cmakedefine CONFIG_DEBUG @CONFIG_DEBUG@

/* Define to 1 if you have the <signal.h> header file. */
#cmakedefine HAVE_SIGNAL_H @HAVE_SIGNAL_H@

/* Define if JACK library is available. */
#cmakedefine CONFIG_JACK @CONFIG_JACK@

/* Define if ALSA library is available. */
#cmakedefine CONFIG_ALSA_SEQ @CONFIG_ALSA_SEQ@

/* Define if PORTAUDIO library is available. */
#cmakedefine CONFIG_PORTAUDIO @CONFIG_PORTAUDIO@

/* Define if jack/statistics.h is available. */
#cmakedefine CONFIG_JACK_STATISTICS @CONFIG_JACK_STATISTICS@

/* Define if CoreAudio/CoreAudio.h is available (Mac OS X). */
#cmakedefine CONFIG_COREAUDIO @CONFIG_COREAUDIO@

/* Define if JACK session support is available. */
#cmakedefine CONFIG_JACK_SESSION @CONFIG_JACK_SESSION@

/* Define if JACK metadata support is available. */
#cmakedefine CONFIG_JACK_METADATA @CONFIG_JACK_METADATA@

/* Define if JACK MIDI support is available. */
#cmakedefine CONFIG_JACK_MIDI @CONFIG_JACK_MIDI@

/* Define if JACK CV support is available. */
#cmakedefine CONFIG_JACK_CV @CONFIG_JACK_CV@

/* Define if JACK OSC support is available. */
#cmakedefine CONFIG_JACK_OSC @CONFIG_JACK_OSC@

/* Define if D-Bus interface is enabled. */
#cmakedefine CONFIG_DBUS @CONFIG_DBUS@

/* Define if unique/single instance is enabled. */
#cmakedefine CONFIG_XUNIQUE @CONFIG_XUNIQUE@

/* Define if debugger stack-trace is enabled. */
#cmakedefine CONFIG_STACKTRACE @CONFIG_STACKTRACE@

/* Define if system tray is enabled. */
#cmakedefine CONFIG_SYSTEM_TRAY @CONFIG_SYSTEM_TRAY@

/* Define if jack_tranport_query is available. */
#cmakedefine CONFIG_JACK_TRANSPORT @CONFIG_JACK_TRANSPORT@

/* Define if jack_is_realtime is available. */
#cmakedefine CONFIG_JACK_REALTIME @CONFIG_JACK_REALTIME@

/* Define if jack_get_xrun_delayed_usecs is available. */
#cmakedefine CONFIG_JACK_XRUN_DELAY @CONFIG_JACK_XRUN_DELAY@

/* Define if jack_get_max_delayed_usecs is available. */
#cmakedefine CONFIG_JACK_MAX_DELAY @CONFIG_JACK_MAX_DELAY@

/* Define if jack_set_port_rename_callback is available. */
#cmakedefine CONFIG_JACK_PORT_RENAME @CONFIG_JACK_PORT_RENAME@

/* Define if jack_port_get_aliases is available. */
#cmakedefine CONFIG_JACK_PORT_ALIASES @CONFIG_JACK_PORT_ALIASES@

/* Define if jack_get_version_string is available. */
#cmakedefine CONFIG_JACK_VERSION @CONFIG_JACK_VERSION@

/* Define if jack_free is available. */
#cmakedefine CONFIG_JACK_FREE @CONFIG_JACK_FREE@


#endif /* CONFIG_H */
