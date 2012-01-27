NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

lib_LTLIBRARIES = libglyphy.la
libglyphy_la_CPPFLAGS = \
	$(NULL)
libglyphy_la_LIBADD = \
	-lm \
	$(NULL)
libglyphy_la_LDFLAGS = \
	-version-info @glyphy_libversion@ \
	$(NULL)
libglyphy_la_SOURCES = \
	glyphy.cc \
	glyphy-arc.cc \
	glyphy-arc-bezier.hh \
	glyphy-arcs.cc \
	glyphy-arcs-bezier.hh \
	glyphy-geometry.hh \
	glyphy-rgba.cc \
	glyphy-rgba.hh \
	$(GLYPHY_PUBLICHEADERS) \
	$(GLYPHY_SHADERHEADERS) \
	$(NULL)
GLYPHY_PUBLICHEADERS = \
	glyphy.h \
	glyphy-arc.h \
	glyphy-freetype.h \
	$(NULL)
GLYPHY_PUBLICSHADERS = \
	glyphy-common.glsl \
	glyphy-sdf.glsl \
	$(NULL)
GLYPHY_SHADERHEADERS = $(patsubst %.glsl,%-glsl.h, $(GLYPHY_PUBLICSHADERS))
BUILT_SOURCES += $(GLYPHY_SHADERHEADERS)

pkginclude_HEADERS = \
	$(GLYPHY_PUBLICHEADERS) \
	$(NULL)
pkgdata_DATA = \
	$(GLYPHY_PUBLICSHADERS) \
	$(NULL)



EXTRA_DIST += stringize
%-glsl.h: %.glsl stringize
	$(AM_V_GEN) ./stringize "static const char *`echo "$<" | sed 's@.*/@@;s/[-.]/_/g'`" < "$<" > "$@.tmp" && \
	mv "$@.tmp" "$@" || ($(RM) "$@.tmp"; false)



check_PROGRAMS = \
	test-geometry \
	$(NULL)
TESTS = \
	test-geometry \
	$(NULL)

test_geometry_SOURCES = test-geometry.cc glyphy.cc



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
	glyphy-demo-gl.h \
	glyphy-demo-glut.h \
	glyphy-demo-shaders.h \
	$(NULL)

endif
endif
endif
endif

-include $(top_srcdir)/git.mk
