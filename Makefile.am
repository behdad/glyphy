NULL =

EXTRA_DIST = \
	autogen.sh \
	ac_define_dir.m4 \
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

SUBDIRS = src demo-gl2 demo-gl3

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glyphy.pc

-include $(top_srcdir)/git.mk
