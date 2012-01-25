NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

lib_LTLIBRARIES = libglyphy.la

libglyphy_la_SOURCES = \
	bezier-arc-approximation.hh \
	glyphy.h \
	glyphy.cc \
	$(NULL)

libglyphy_la_CPPFLAGS = \
	$(NULL)
libglyphy_la_LIBADD = \
	-lm \
	$(NULL)
libglyphy_la_LDFLAGS = \
	-version-info @glyphy_libversion@ \
	$(NULL)

pkginclude_HEADERS = \
	glyphy.h \
	$(NULL)



check_PROGRAMS = \
	test-geometry \
	$(NULL)
TESTS = \
	test-geometry \
	$(NULL)

test_geometry_SOURCES = test-geometry.cc glyphy.cc bezier-arc-approximation.hh



if HAVE_FREETYPE2
if HAVE_GL
if HAVE_GLEW
if HAVE_GLUT

noinst_PROGRAMS = glyphy-demo

glyphy_demo_CPPFLAGS = \
	$(FREETYPE2_CFLAGS) \
	$(GL_CFLAGS) \
	$(GLEW_CFLAGS) \
	$(GLUT_CFLAGS) \
	$(NULL)
glyphy_demo_LDADD = \
	libglyphy.la \
	-lm \
	$(FREETYPE2_LIBS) \
	$(GL_LIBS) \
	$(GLEW_LIBS) \
	$(GLUT_LIBS) \
	$(NULL)
glyphy_demo_SOURCES = \
	glyphy-demo.cc \
	$(NULL)

endif
endif
endif
endif

-include $(top_srcdir)/git.mk
