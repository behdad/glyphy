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

if HAVE_CAIRO_FT
if HAVE_GTK_2_0
if HAVE_GDK_X11_2_0
if HAVE_EGL
if HAVE_GLESV2

bin_PROGRAMS += glyphy

glyphy_CPPFLAGS = \
		$(CAIRO_FT_CFLAGS) \
		$(GTK_2_0_CFLAGS) \
		$(GDK_X11_2_0_CFLAGS) \
		$(EGL_CFLAGS) \
		$(GLESV2_CFLAGS) \
		$(NULL)
glyphy_LDADD = \
		$(CAIRO_FT_LIBS) \
		$(GTK_2_0_LIBS) \
		$(GDK_X11_2_0_LIBS) \
		$(EGL_LIBS) \
		$(GLESV2_LIBS) \
		-lm \
		$(NULL)
glyphy_SOURCES = \
		glyphy.cc \
		$(NULL)

endif
endif
endif
endif
endif

-include $(top_srcdir)/git.mk
