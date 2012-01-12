
if HAVE_FREETYPE2
if HAVE_GL
if HAVE_GLEW
if HAVE_GLUT

noinst_PROGRAMS = glyphyDemo

glyphyDemo_CPPFLAGS = \
		$(FREETYPE2_CFLAGS) \
		$(GL_CFLAGS) \
		$(GLEW_CFLAGS) \
		$(GLUT_CFLAGS) \
		-I$(top_srcdir)/include \
		$(NULL)
glyphyDemo_LDADD = \
		$(FREETYPE2_LIBS) \
		$(GL_LIBS) \
		$(GLEW_LIBS) \
		$(GLUT_LIBS) \
		-lm \
		-lglyphy-0.1 \
		-L$(top_builddir)/src \
		$(NULL)
glyphyDemo_SOURCES = \
		glyphy_demo.cc \
		$(NULL)

endif
endif
endif
endif
