AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/oracle-config.h \
	src/oracle.h \
        src/oracleobject.h \
	src/oracle-module.h \
	src/QoreOracleStatement.h \
	src/QorePreparedStatement.h \
	src/QoreOracleConnection.h \
	src/OraColumnValue.h \
	src/OraResultSet.h \
	src/ocilib/oci_api.h \
	src/ocilib/ocilib_checks.h \
	src/ocilib/ocilib_internal.h \
	src/ocilib/oci_types.h \
	src/ocilib/oci_defs.h \
	src/ocilib/ocilib_defs.h \
	src/ocilib/ocilib_types.h \
	src/ocilib/oci_import.h \
	src/ocilib/ocilib.h \
	src/ocilib/oci_loader.h


EXTRA_DIST = COPYING ChangeLog AUTHORS README \
	RELEASE-NOTES \
	test/db-test.q \
	test/stmt.q \
	docs/oracle-module-doc.html \
	qore-oracle-module.spec

ACLOCAL_AMFLAGS=-I m4

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
