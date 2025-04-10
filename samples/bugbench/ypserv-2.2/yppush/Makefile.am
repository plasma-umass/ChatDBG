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
	-DCONFDIR=\"$(sysconfdir)\" -DYPMAPDIR=\"/var/yp\" \
	-DUSE_FQDN=@USE_FQDN@
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir) -I$(top_builddir) -I$(srcdir)

CLEANFILES = *~ ${MANS}

EXTRA_DIST = $(addsuffix .in,$(MANS))

man_MANS = yppush.8

sbin_PROGRAMS = yppush

yppush_SOURCES = yppush.c

yppush_LDADD =  @LIBDBM@  $(top_builddir)/lib/libyp.a

