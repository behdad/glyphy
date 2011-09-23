PKGS = gtk+-2.0 cairo

CPPFLAGS = `pkg-config --cflags ${PKGS}`
LDFLAGS = `pkg-config --libs ${PKGS}`

all: arc

clean:
	$(RM) arc
