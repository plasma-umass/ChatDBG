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
	-DUSE_FQDN=@USE_FQDN@
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir) -I$(top_builddir) -I$(srcdir)

CLEANFILES = *~ ${MANS}

EXTRA_DIST = $(addsuffix .in,$(MANS))

noinst_HEADERS = yppasswd.h yppasswd.x

man_MANS = yppasswdd.8 rpc.yppasswdd.8

sbin_PROGRAMS = rpc.yppasswdd

rpc_yppasswdd_SOURCES = update.c yppasswd_xdr.c yppasswdd.c

rpc_yppasswdd_LDADD =  $(LIBDBM) $(LIBCRYPT) $(top_builddir)/lib/libyp.a

