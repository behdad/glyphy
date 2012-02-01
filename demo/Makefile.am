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
	demo-atlas.h \
	demo-atlas.cc \
	demo-buffer.h \
	demo-buffer.cc \
	demo-font.h \
	demo-font.cc \
	demo-shader.h \
	demo-shader.cc \
	glyphy-demo.h \
	glyphy-demo.cc \
	glyphy-demo-animation.cc \
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

EXTRA_DIST += TEXT

-include $(top_srcdir)/git.mk
