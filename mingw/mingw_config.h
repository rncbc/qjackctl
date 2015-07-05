/* src/config.h.  Generated from config.h.in by configure.  */
/* src/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if ALSA/MIDI sequencer support is enabled. */
//#define CONFIG_ALSA_SEQ 1

/* Define if CoreAudio/CoreAudio.h is available (Mac OS X). */
/* #undef CONFIG_COREAUDIO */

/* Define if D-Bus interface is enabled. */
//#define CONFIG_DBUS 1

/* Define if debugging is enabled. */
/* #undef CONFIG_DEBUG */

/* Define if jack_free is available. */
#define CONFIG_JACK_FREE 1

/* Define if jack_get_max_delayed_usecs is available. */
#define CONFIG_JACK_MAX_DELAY 1

/* Define if JACK metadata support is available. */
//#define CONFIG_JACK_METADATA 1

/* Define if JACK MIDI support is available. */
#define CONFIG_JACK_MIDI 1

/* Define if jack_port_get_aliases is available. */
#define CONFIG_JACK_PORT_ALIASES 1

/* Define if jack_is_realtime is available. */
#define CONFIG_JACK_REALTIME 1

/* Define if JACK session support is available. */
//#define CONFIG_JACK_SESSION 1

/* Define if jack/statistics.h is available. */
#define CONFIG_JACK_STATISTICS 1

/* Define if jack_tranport_query is available. */
#define CONFIG_JACK_TRANSPORT 1

/* Define if jack_get_version_string is available. */
/* #undef CONFIG_JACK_VERSION */

/* Define if jack_get_xrun_delayed_usecs is available. */
#define CONFIG_JACK_XRUN_DELAY 1

/* Define if PortAudio interface is enabled. */
/* #undef CONFIG_PORTAUDIO */
#define CONFIG_PORTAUDIO 1

/* Default installation prefix. */
#define CONFIG_PREFIX "/usr/local"

/* Define if round is available. */
#define CONFIG_ROUND 1

/* Define if debugger stack-trace is enabled. */
/* #undef CONFIG_STACKTRACE */

/* Define if system tray is enabled. */
#define CONFIG_SYSTEM_TRAY 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `X11' library (-lX11). */
//#define HAVE_LIBX11 1

/* Define to 1 if you have the `Xext' library (-lXext). */
//#define HAVE_LIBXEXT 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `system' function. */
#define HAVE_SYSTEM 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "rncbc@rncbc.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "QjackCtl"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "QjackCtl 0.3.13.10"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "qjackctl"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.3.13.10"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1
