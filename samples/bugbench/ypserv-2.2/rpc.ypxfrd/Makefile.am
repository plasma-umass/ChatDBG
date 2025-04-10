#
# Copyright (c) 2001 Thorsten Kukuk <kukuk@suse.de>
#
AUTOMAKE_OPTIONS = 1.4 gnits
MAINT_CHARSET = latin1
#
INSTALL_PROGRAM = @INSTALL_PROGRAM@ -s -m 555

localedir = $(datadir)/locale

CFLAGS = @EXTRA_CFLAGS@ @CFLAGS@
DEFS = @DEFS@ -DLOCALEDIR=\"$(localedir)\" -DYPBINDIR=\"$(libexecdir)\" \
	-DCONFDIR=\"$(sysconfdir)\" -DYPMAPDIR=\"@YPMAPDIR@\" \
	-DUSE_FQDN=@USE_FQDN@ -DXFRBLOCKSIZE=65535
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir) -I$(top_builddir) -I$(srcdir)

CLEANFILES = *~ ${MANS}

EXTRA_DIST = $(addsuffix .in,$(MANS))

noinst_HEADERS = ypxfrd.h

man_MANS = ypxfrd.8 rpc.ypxfrd.8

sbin_PROGRAMS = rpc.ypxfrd

rpc_ypxfrd_SOURCES = ypxfrd.c ypxfrd_server.c ypxfrd_svc.c

rpc_ypxfrd_LDADD =  $(top_builddir)/lib/libyp.a  @LIBDBM@
