AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/oracle-config.h \
	src/oracle.h \
	src/oracle-module.h

EXTRA_DIST = COPYING ChangeLog AUTHORS README \
	RELEASE-NOTES \
	test/db-test.q \
	docs/oracle-module-doc.html \
	qore-oracle-module.spec

ACLOCAL_AMFLAGS=-I m4

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
