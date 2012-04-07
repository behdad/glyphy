AC_PREREQ([2.64])
AC_INIT([glyphy],
        [0.2.0],
        [http://code.google.com/p/glyphy/issues/list],
        [glyphy],
        [http://code.google.com/p/glyphy/])

AC_CONFIG_SRCDIR([glyphy.pc.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11.1 gnu dist-bzip2 no-dist-gzip -Wall])
AM_SILENT_RULES([yes])

AC_CANONICAL_HOST

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

	m4_foreach_w(lib, [m4_translit(PKG,[A-Z],[a-z]) PKG], [
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
GLYPHY_CHECK_PACKAGE(cairo, cairo_create)
GLYPHY_CHECK_PACKAGE(cairo-ft, cairo_ft_scaled_font_lock_face)
GLYPHY_CHECK_PACKAGE(gl, glCreateShader, OpenGL)
GLYPHY_CHECK_PACKAGE(glew, glewInit)
GLYPHY_CHECK_PACKAGE(glut, glutInit, GLUT)

dnl ===========================================================================

# Find default font, for the demo
AC_MSG_CHECKING([for default font])
DEFAULT_FONT="`fc-match -v 2>/dev/null | grep 'file:' | cut -d'\"' -f2`"
if test "x$DEFAULT_FONT" = x; then :; else
	AC_DEFINE_UNQUOTED([DEFAULT_FONT], ["$DEFAULT_FONT"], [Default font file.])
fi
AC_MSG_RESULT($DEFAULT_FONT)

dnl ===========================================================================

AC_DEFINE_DIR([PKGDATADIR], [$pkgdatadir], [Define to the directory containing package data.])

AC_CONFIG_FILES([
glyphy.pc
Makefile
src/Makefile
demo/Makefile
])

AC_OUTPUT
