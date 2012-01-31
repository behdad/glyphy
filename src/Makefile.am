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
	glyphy-common.hh \
	glyphy-extents.cc \
	glyphy-geometry.hh \
	glyphy-rgba.cc \
	glyphy-shaders.cc \
	$(PUBLICHEADERS) \
	$(SHADERHEADERS) \
	$(NULL)
PUBLICHEADERS = \
	glyphy.h \
	glyphy-freetype.h \
	$(NULL)
SHADERS = \
	glyphy-common.glsl \
	glyphy-sdf.glsl \
	$(NULL)
SHADERHEADERS = $(patsubst %.glsl,%-glsl.h, $(SHADERS))
BUILT_SOURCES += $(SHADERHEADERS)
EXTRA_DIST += $(SHADERS)

pkginclude_HEADERS = \
	$(PUBLICHEADERS) \
	$(NULL)
pkgdata_DATA = \
	$(SHADERS) \
	$(NULL)

EXTRA_DIST += stringize
%-glsl.h: %.glsl stringize
	$(AM_V_GEN) ./stringize "static const char *`echo "$<" | \
	sed 's@.*/@@;s/[-.]/_/g'`" < "$<" > "$@.tmp" && \
	mv "$@.tmp" "$@" || ($(RM) "$@.tmp"; false)


-include $(top_srcdir)/git.mk
