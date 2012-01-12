NULL =

EXTRA_DIST = \
	autogen.sh \
	autogen_no_autoheader.sh \
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

bin_PROGRAMS =

SUBDIRS = include src demo

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glyphy-@PACKAGE_VERSION@.pc

-include $(top_srcdir)/git.mk
