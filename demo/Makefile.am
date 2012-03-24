NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
BUILT_SOURCES =
noinst_PROGRAMS =


if HAVE_FREETYPE2
if HAVE_GL
if HAVE_GLEW
if HAVE_GLUT

noinst_PROGRAMS += glyphy-demo

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
	demo-atlas.h \
	demo-atlas.cc \
	demo-buffer.h \
	demo-buffer.cc \
	demo-common.h \
	demo-font.h \
	demo-font.cc \
	demo-shader.h \
	demo-shader.cc \
	demo-view.h \
	demo-view.cc \
	glyphy-demo.cc \
	trackball.h \
	trackball.c \
	$(SHADERHEADERS) \
	$(NULL)
SHADERS = \
	demo-atlas.glsl \
	demo-fshader.glsl \
	demo-vshader.glsl \
	$(NULL)
SHADERHEADERS = $(patsubst %.glsl,%-glsl.h, $(SHADERS))
BUILT_SOURCES += $(SHADERHEADERS)
EXTRA_DIST += $(SHADERS)

%-glsl.h: %.glsl $(top_srcdir)/src/stringize
	$(AM_V_GEN) $(top_srcdir)/src/stringize "static const char *`echo "$<" | \
	sed 's@.*/@@;s/[-.]/_/g'`" < "$<" > "$@.tmp" && \
	mv "$@.tmp" "$@" || ($(RM) "$@.tmp"; false)

endif
endif
endif
endif


if HAVE_FREETYPE2

noinst_PROGRAMS += glyphy-validate
glyphy_validate_CPPFLAGS = \
	-I $(top_srcdir)/src \
	$(FREETYPE2_CFLAGS) \
	$(NULL)
glyphy_validate_LDADD = \
	$(top_builddir)/src/libglyphy.la \
	$(FREETYPE2_LIBS) \
	$(NULL)
glyphy_validate_SOURCES = \
	demo-common.h \
	glyphy-validate.cc \
	$(NULL)

endif


EXTRA_DIST += TEXT

-include $(top_srcdir)/git.mk
