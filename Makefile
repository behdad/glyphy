PKGS = glesv2 egl x11 cairo gdk-x11-2.0 gtk+-2.0 cairo

CPPFLAGS = `pkg-config --cflags ${PKGS}`
LDFLAGS = `pkg-config --libs ${PKGS}`

all: gltext
