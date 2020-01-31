
#ifndef LIBFREENECT2_EXPORT_H
#define LIBFREENECT2_EXPORT_H

#ifdef LIBFREENECT2_STATIC_DEFINE
#  define LIBFREENECT2_EXPORT
#  define LIBFREENECT2_NO_EXPORT
#else
#  ifndef LIBFREENECT2_EXPORT
#    ifdef freenect2_EXPORTS
        /* We are building this library */
#      define LIBFREENECT2_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define LIBFREENECT2_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef LIBFREENECT2_NO_EXPORT
#    define LIBFREENECT2_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef LIBFREENECT2_DEPRECATED
#  define LIBFREENECT2_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef LIBFREENECT2_DEPRECATED_EXPORT
#  define LIBFREENECT2_DEPRECATED_EXPORT LIBFREENECT2_EXPORT LIBFREENECT2_DEPRECATED
#endif

#ifndef LIBFREENECT2_DEPRECATED_NO_EXPORT
#  define LIBFREENECT2_DEPRECATED_NO_EXPORT LIBFREENECT2_NO_EXPORT LIBFREENECT2_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LIBFREENECT2_NO_DEPRECATED
#    define LIBFREENECT2_NO_DEPRECATED
#  endif
#endif

#endif
