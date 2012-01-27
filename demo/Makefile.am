NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
BUILT_SOURCES =


if HAVE_FREETYPE2
if HAVE_GL
if HAVE_GLEW
if HAVE_GLUT

noinst_PROGRAMS = glyphy-demo

glyphy_demo_CPPFLAGS = \
	-I $(top_srcdir)/src \
	$(FREETYPE2_CFLAGS) \
	$(GL_CFLAGS) \
	$(GLEW_CFLAGS) \
	$(GLUT_CFLAGS) \
	$(NULL)
glyphy_demo_LDADD = \
	$(top_builddir)/src/libglyphy.la \
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
