AC_PREREQ([2.64])
AC_INIT([glyphy],
        [0.2.0],
        [http://code.google.com/p/glyphy/issues/list],
        [glyphy],
        [http://code.google.com/p/glyphy/])

AC_CONFIG_SRCDIR([glyphy.pc.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11.1 foreign dist-bzip2 no-dist-gzip -Wall])
AM_SILENT_RULES([yes])

AC_CANONICAL_HOST

android=false
case $host in
	*-android*) android=true;;
esac
AM_CONDITIONAL(ANDROID, $android)

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# TODO flesh this out
GLYPHY_LIBTOOL_VERSION_INFO="0:0:0"
AC_SUBST(GLYPHY_LIBTOOL_VERSION_INFO)

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
PKG_PROG_PKG_CONFIG

dnl ==========================================================================

dnl syntax: pkgname, symbol, framework
AC_DEFUN([GLYPHY_CHECK_PACKAGE],
[
	m4_define([pkg], [$1])
	m4_define([_symbol], [$2])
	m4_define([_framework], [$3])
	m4_define([_more], [$4])
	m4_define([PKG], [m4_translit(pkg,[-a-z.+],[_A-Z_])])

	HAVE_[]PKG=false

	if test -n "_framework"; then
		case "$host" in
			*-darwin*)
				AC_MSG_CHECKING([for ]PKG)
				HAVE_[]PKG=true
				PKG[]_LIBS="-framework _framework"
				AC_MSG_RESULT($PKG[]_LIBS)
				break;
			;;
		esac
	fi

	if ! $HAVE_[]PKG; then
		PKG_CHECK_MODULES(PKG, pkg, [HAVE_]PKG[=true], [HAVE_]PKG[=false])
	fi

	m4_foreach_w(lib, [m4_translit(PKG,[A-Z],[a-z]) PKG _more], [
		if ! $HAVE_[]PKG; then
			AC_CHECK_LIB(lib, _symbol, [
						    HAVE_]PKG[=true
						    ]PKG[_LIBS=-l]lib[
						   ])
		fi
	])
	if [$HAVE_]PKG; then AC_DEFINE([HAVE_]PKG, 1, [Have ]pkg[ library]) fi
	AM_CONDITIONAL([HAVE_]PKG, [$HAVE_]PKG)
])

GLYPHY_CHECK_PACKAGE(freetype2, FT_Init_FreeType)
if $android; then
	GLYPHY_CHECK_PACKAGE(gl, glCreateShader,, GLESv2)
	GLYPHY_CHECK_PACKAGE(glew, glewInit)
	GLYPHY_CHECK_PACKAGE(glut, glutInit,, freeglut-gles2)
	GLUT_CFLAGS=-DFREEGLUT_GLES2
	AC_SUBST(GLUT_CFLAGS)
	AC_DEFINE([__ANDROID__], 1, [On Android])
else
	GLYPHY_CHECK_PACKAGE(gl, glCreateShader, OpenGL)
	GLYPHY_CHECK_PACKAGE(glew, glewInit)
	GLYPHY_CHECK_PACKAGE(glut, glutInit, GLUT)
fi

dnl ===========================================================================

AC_DEFINE_DIR([PKGDATADIR], [$pkgdatadir], [Define to the directory containing package data.])

AC_CONFIG_FILES([
glyphy.pc
Makefile
src/Makefile
demo-gl2/Makefile
demo-gl2/android/Makefile
demo-gl3/Makefile
demo-gl3/android/Makefile
])

AC_OUTPUT
