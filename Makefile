GLPKGS = glesv2 egl x11 cairo gdk-x11-2.0 gtk+-2.0 cairo
PKGS = cairo freetype2

CPPFLAGS = `pkg-config --cflags ${PKGS} ${GLPKGS}`
LDFLAGS = `pkg-config --libs ${PKGS} ${GLPKGS}` -lm

all: arc grid gltext

grid: grid.cc geometry.hh cairo-helper.hh freetype-helper.hh sample-curves.hh bezier-arc-approximation.hh
arc: arc.cc geometry.hh cairo-helper.hh freetype-helper.hh sample-curves.hh bezier-arc-approximation.hh
