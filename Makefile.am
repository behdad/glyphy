NULL =

EXTRA_DIST = \
	autogen.sh \
	$(NULL)

MAINTAINERCLEANFILES = \
	$(srcdir)/INSTALL \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/autoscan.log \
	$(srcdir)/compile \
	$(srcdir)/config.guess \
	$(srcdir)/config.h.in \
	$(srcdir)/config.sub \
	$(srcdir)/configure.scan \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/missing \
	$(srcdir)/mkinstalldirs \
	$(srcdir)/ChangeLog \
	`find "$(srcdir)" -type f -name Makefile.in -print`


bin_PROGRAMS =

if HAVE_CAIRO_FT

bin_PROGRAMS += arc grid

arc_CPPFLAGS = $(CAIRO_FT_CFLAGS)
arc_LDADD = $(CAIRO_FT_LIBS) -lm
arc_SOURCES = \
		arc.cc \
		bezier-arc-approximation.hh \
		cairo-helper.hh \
		freetype-helper.hh \
		geometry.hh \
		sample-curves.hh \
		$(NULL)

grid_CPPFLAGS = $(CAIRO_FT_CFLAGS)
grid_LDADD = $(CAIRO_FT_LIBS) -lm
grid_SOURCES = \
		grid.cc \
		bezier-arc-approximation.hh \
		cairo-helper.hh \
		freetype-helper.hh \
		geometry.hh \
		sample-curves.hh \
		$(NULL)

endif

if HAVE_FREETYPE2
if HAVE_GL
if HAVE_GLEW
if HAVE_GLUT

bin_PROGRAMS += glyphy

glyphy_CPPFLAGS = \
		$(FREETYPE2_CFLAGS) \
		$(GL_CFLAGS) \
		$(GLEW_CFLAGS) \
		$(GLUT_CFLAGS) \
		$(NULL)
glyphy_LDADD = \
		$(FREETYPE2_LIBS) \
		$(GL_LIBS) \
		$(GLEW_LIBS) \
		$(GLUT_LIBS) \
		-lm \
		$(NULL)
glyphy_SOURCES = \
		glyphy.cc \
		$(NULL)

endif
endif
endif
endif

-include $(top_srcdir)/git.mk
