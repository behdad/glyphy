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
	glyphy-arc.cc \
	glyphy-arc-bezier.hh \
	glyphy-arcs.cc \
	glyphy-arcs-bezier.hh \
	glyphy-geometry.hh \
	glyphy-rgba.cc \
	glyphy-rgba.hh \
	glyphy-shaders.c \
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
EXTRA_DIST += $(GLYPHY_PUBLICSHADERS)

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


-include $(top_srcdir)/git.mk
