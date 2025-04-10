#
# Copyright (c) 2000, 2001 Thorsten Kukuk <kukuk@suse.de>
#
AUTOMAKE_OPTIONS = 1.4 gnits
MAINT_CHARSET = latin1
#
INSTALL_PROGRAM = @INSTALL_PROGRAM@ -s -m 555

localedir = $(datadir)/locale

CFLAGS = @EXTRA_CFLAGS@ @CFLAGS@
DEFS = @DEFS@ -DLOCALEDIR=\"$(localedir)\" -DYPBINDIR=\"$(libexecdir)\" \
	-DYPMAPDIR=\"@YPMAPDIR@\"
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir) -I$(top_builddir) -I$(srcdir)

CLEANFILES = *~

EXTRA_DIST = $(addsuffix .in,$(MANS))

man_MANS = ypserv.8

noinst_HEADERS =

sbin_PROGRAMS = ypserv

ypserv_SOURCES = ypserv.c server.c ypserv_xdr.c

ypserv_LDADD =  @LIBDBM@ $(top_builddir)/lib/libyp.a
