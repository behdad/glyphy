PLATFORM_PREFIX := /home/behdad/.local/arm-linux-androideabi

APP_STL := gnustl_static

APP_CPPFLAGS := \
	-I$(PLATFORM_PREFIX)/include/ \
	-I$(PLATFORM_PREFIX)/include/freetype2 \
	-I$(PLATFORM_PREFIX)/include/glyphy \
	-DDEFAULT_FONT='"/system/fonts/Roboto-Regular.ttf"' \
	-DFREEGLUT_GLES2 \
	$(NULL)
