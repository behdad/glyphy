AM_CXXFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libglyphy-0.1.la

libglyphy_0_1_la_CPPFLAGS = \
		$(FREETYPE2_CFLAGS) \
		$(GL_CFLAGS) \
		$(GLEW_CFLAGS) \
		$(GLUT_CFLAGS) \
		$(NULL)
libglyphy_0_1_la_LIBADD = \
		$(FREETYPE2_LIBS) \
		$(GL_LIBS) \
		$(GLEW_LIBS) \
		$(GLUT_LIBS) \
		-lm \
		$(NULL)
libglyphy_0_1_la_LDFLAGS = \
		-version-info @glyphy_libversion@ \
		$(NULL)
libglyphy_0_1_la_SOURCES = \
	bezier-arc-approximation.hh \
	cairo-helper.hh \
	glyphy_freetype.cc \
	glyphy_lib.cc \
	sample-curves.hh \
	$(NULL)
