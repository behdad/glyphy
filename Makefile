PKGS = gtk+-2.0 cairo

CPPFLAGS = `pkg-config --cflags ${PKGS}`
LDFLAGS = `pkg-config --libs ${PKGS}`

all: arc

arc: arc.cc geometry.hh

clean:
	$(RM) arc
