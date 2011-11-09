AC_PREREQ([2.64])
AC_INIT([glyphy],
        [0.0.0],
        [http://code.google.com/p/glyphy/issues/list],
        [glyphy],
        [http://code.google.com/p/glyphy/])

AC_CONFIG_SRCDIR([geometry.hh])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11.1 dist-bzip2 no-dist-gzip -Wall])
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
PKG_PROG_PKG_CONFIG

dnl ==========================================================================

m4_foreach_w([pkg], [cairo freetype2 cairo-ft gdk-x11-2.0 gtk+-2.0 egl glesv2], [
	m4_define([PKG], m4_translit(pkg,[-a-z.+],[_A-Z_]))
	PKG_CHECK_MODULES(PKG, pkg, [have=true], [have=false])
	if $have; then
		AC_DEFINE([HAVE_]PKG, 1, [Have ]pkg[ library])
	fi
	AM_CONDITIONAL([HAVE_]PKG, $have)
])

dnl ===========================================================================

AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT
