/* config.h.  Pre-canned for VS.  */

/* Default font file. */
#define DEFAULT_FONT "Calibri"

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Have freetype2 library */
/* #undef HAVE_FREETYPE2 */

/* Have gl library */
#define HAVE_GL 1

/* Have glew library */
#define HAVE_GLEW 1

/* Have glut library */
#define HAVE_GLUT1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "glyphy"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://code.google.com/p/glyphy/issues/list"

/* Define to the full name of this package. */
#define PACKAGE_NAME "glyphy"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "glyphy 0.2.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "glyphy"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://code.google.com/p/glyphy/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.2.0"

/* Define to the directory containing package data. */
#define PKGDATADIR "../src"

/* Define to 1 if you have the ANSI C header files. */
/* #undef STDC_HEADERS */

/* Version number of package */
#define VERSION "0.2.0"

/* On Android */
/* #undef __ANDROID__ */

/* http://stackoverflow.com/questions/70013/how-to-detect-if-im-compiling-code-with-visual-studio-2008
MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio 2003)
MSVC++ 7.0  _MSC_VER == 1300
MSVC++ 6.0  _MSC_VER == 1200
MSVC++ 5.0  _MSC_VER == 1100
*/

#if defined(_MSC_VER) && _MSC_VER <= 1700 /*  */

#include <cmath>
#include <intrin.h>
#include <stdint.h>

#define isnan(x) _isnan(x)
#define log2 __lzcnt
#define round(x) floor((x) + 0.5) 

inline int isinf(double x) {
    union { uint64_t u; double f; } ieee754;
    ieee754.f = x;
    return ((uint32_t)(ieee754.u >> 32) & 0x7fffffff ) == 0x7ff00000 && (uint32_t)ieee754.u == 0;
}

inline int lround(double d) { return static_cast<int>(d + 0.5); }

namespace std {
    inline int lround(double d) { return ::lround(d); }
}

#ifndef INIFINITY
#ifndef _HUGE_ENUF
    #define _HUGE_ENUF  1e+300  // _HUGE_ENUF*_HUGE_ENUF must overflow
#endif
#define INFINITY   ((float)(_HUGE_ENUF * _HUGE_ENUF))
#endif

template<typename T> bool isfinite(T arg)
{
    return arg == arg && 
           arg != std::numeric_limits<T>::infinity() &&
           arg != -std::numeric_limits<T>::infinity();
}

#endif
