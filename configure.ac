AC_PREREQ([2.64])
AC_INIT([glyphy],
        [0.0.0],
        [http://code.google.com/p/glyphy/issues/list],
        [glyphy],
        [http://code.google.com/p/glyphy/])

AC_CONFIG_SRCDIR([include/glyphy/geometry.hh])
AC_CONFIG_HEADERS([include/glyphy/config.h])

AM_INIT_AUTOMAKE([1.11.1 dist-bzip2 no-dist-gzip -Wall])
AM_SILENT_RULES([yes])
AM_PROG_LIBTOOL
LT_INIT

AC_CANONICAL_HOST

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

	PKG_CHECK_MODULES(PKG, pkg, [HAVE_]PKG[=true], [HAVE_]PKG[=false])

	if ! $HAVE_[]PKG && test -n "_framework"; then
		case "$host" in
			*-darwin*)
			    HAVE_[]PKG=true
			    PKG[]_LIBS="-framework _framework"
			    break;
			;;
		esac
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

glyphy_includedir=${includedir}/${PACKAGE_NAME}-${PACKAGE_VERSION}
AC_SUBST(glyphy_includedir)

glyphy_datadir=${datadir}/${PACKAGE_NAME}-${PACKAGE_VERSION}
AC_SUBST(glyphy_datadir)

glyphy_libversion="0:0:0"
AC_SUBST(glyphy_libversion)

AC_CONFIG_FILES([ Makefile
		  include/Makefile
		  src/Makefile
		  demo/Makefile
		  glyphy-${PACKAGE_VERSION}.pc ])

AC_DEFINE_DIR([GLYPHY_SHADERDIR], [glyphy_datadir], [Directory containing shader code.])

AC_OUTPUT
