SUBDIRS = src

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = paukn.pc

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

#tell aclocal that extra macros can be found in m4/
ACLOCAL_AMFLAGS = -I m4
