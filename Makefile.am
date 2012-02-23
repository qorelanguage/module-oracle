AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

DOXYGEN_QORE_API_FILTER=docs/doxygen-api-filter.q
DOXYGEN_OUTPUT=docs/oracle

DOXYFILES = docs/doxyfile \
	docs/mainpage.doxygen

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
	test/sql-stmt.q \
	qore-oracle-module.spec

ACLOCAL_AMFLAGS=-I m4

if COND_DOXYGEN
DX_CLEANFILES = ${DOXYFILES}
docs/oracle/html: $(DOXYFILES)
	cd docs; $(DOXYGEN_CMD) doxyfile;
	$(DOXYGEN_QORE_API_FILTER) --post $(DOXYGEN_OUTPUT)/html/*html
	$(DOXYGEN_QORE_API_FILTER) --post $(DOXYGEN_OUTPUT)/html/search/*html

doxygen-doc: docs/oracle/html

EXTRA_DIST += docs/oracle/html

else
doxygen-doc:
	echo no doxygen support
endif

mostlyclean-local:
	rm -rf ${DOXYGEN_OUTPUT} ${DX_CLEANFILES}

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

docs/mainpage.doxygen: docs/mainpage.doxygen.tmpl
	$(DOXYGEN_QORE_API_FILTER) -l $<

docs/doxyfile: docs/doxyfile.tmpl
	sed s/__VERSION__/${VERSION}/ $< > $@
