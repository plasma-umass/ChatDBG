#
# Copyright (c) 2001 Thorsten Kukuk <kukuk@suse.de>
#
AUTOMAKE_OPTIONS = 1.4 gnits
MAINT_CHARSET = latin1
#
INSTALL_SCRIPT = ${INSTALL} -m 755

CLEANFILES = *~ ${SCRIPTS}

EXTRA_DIST = $(addsuffix .in,$(MANS)) $(addsuffix .in,$(SCRIPTS))

varypdir = /var/yp

man_MANS = pwupdate.8 ypinit.8

libexec_SCRIPTS = ypxfr_1perhour ypxfr_1perday ypxfr_2perday \
		create_printcap match_printcap pwupdate ypinit

noinst_SCRIPTS = ypMakefile

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(varypdir)
	$(INSTALL_DATA) $(top_builddir)/scripts/ypMakefile $(DESTDIR)$(varypdir)/Makefile

