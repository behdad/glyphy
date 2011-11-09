NULL =
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
