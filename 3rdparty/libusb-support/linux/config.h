/* Default visibility */
#define DEFAULT_VISIBILITY __attribute__((visibility("default")))

/* Message logging */
#define ENABLE_LOGGING 1

/* Uncomment to start with debug message logging enabled */
// #define ENABLE_DEBUG_LOGGING 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `udev' library (-ludev). */
/* #undef HAVE_LIBUDEV */

/* Define to 1 if you have the <libudev.h> header file. */
/* #undef HAVE_LIBUDEV_H */

/* Define to 1 if you have the <linux/filter.h> header file. */
/* #undef HAVE_LINUX_FILTER_H */

/* Define to 1 if you have the <linux/netlink.h> header file. */
#define HAVE_LINUX_NETLINK_H 1

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

/* Define to 1 if the system has the type `struct timespec'. */
#define HAVE_STRUCT_TIMESPEC 1

/* syslog() function available */
#define HAVE_SYSLOG_FUNC 1

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
/* #undef HAVE_SYS_SOCKET_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

#ifdef _OSX_
/* Darwin backend for Mac */
#define OS_DARWIN 1
#else
/* Linux backend for Android and generic Linux */
#define OS_LINUX 1
#endif /* _OSX */

/* Haiku backend */
/* #undef OS_HAIKU */

/* NetBSD backend */
/* #undef OS_NETBSD */

/* OpenBSD backend */
/* #undef OS_OPENBSD */

/* Windows backend */
/* #undef OS_WINDOWS */

/* Name of package */
#define PACKAGE "libusb"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "libusb-devel@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libusb"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libusb 1.0.23"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libusb"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://libusb.info"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.0.23"

/* type of second poll() argument */
#define POLL_NFDS_TYPE nfds_t

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Use POSIX Threads */
#define THREADS_POSIX 1

/* timerfd headers available */
/* Not when configure was run on a 10.6.8 MacBook Pro. */
/* Not when configure was run with arm-linux-androideabi-4.4.3 */
/* Not when building on Ubuntu 8.04. */
/* Even if TimerFD is available, don't use it because it forces a
 * dependency on GLibC > 2.9.  See Sharepoint bug 207 and Bugzilla bug 57.
 */
#undef USBI_TIMERFD_AVAILABLE

/* Enable output to system log */
/* #undef USE_SYSTEM_LOGGING_FACILITY */

/* Use udev for device enumeration/hotplug */
/* #undef USE_UDEV */

/* Version number of package */
#define VERSION "1.0.20"

/* Oldest Windows version supported */
/* #undef WINVER */

/* Use GNU extensions */
#define _GNU_SOURCE 1

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Building on Mac OS 10.6.8 reveals darwin_usb.c omits to include inttypes */
#ifndef PRIx64
#include <inttypes.h>
#endif /* PRIx64 */

#ifdef FORTIFY
/* Fortify (memory checking tool) enabled so all sources must include
 * fortify.h.  Do it here to avoid modifying libusb sources.
 */
#include "fortify.h"
#endif /* FORTIFY */

#ifdef __ANDROID__
/* Quick and dirty way to redirect libusb's debug functions to Android's 
 * logger.  You also need to prefix all fprintf and vfprintf in libusb/core.c 
 * with 'z', a change which you should not check in!
 */
#include <android/log.h>
#define zfprintf(stream, format, args ...) \
	__android_log_print(ANDROID_LOG_DEBUG, "libusbx", format, ##args)
#define zvfprintf(stream, format, arglist) \
	__android_log_vprint(ANDROID_LOG_DEBUG, "libusbx", format, arglist)

/* Android's time.h has no definition of TIMESPEC_TO_TIMEVAL */
#ifndef TIMESPEC_TO_TIMEVAL
#define TIMESPEC_TO_TIMEVAL(tv, ts)       \
do                                        \
{                                         \
    (tv)->tv_sec = (ts)->tv_sec;          \
    (tv)->tv_usec = (ts)->tv_nsec / 1000; \
}                                         \
while (0)
#endif /* TIMESPEC_TO_TIMEVAL */

#endif /* __ANDROID__ */
