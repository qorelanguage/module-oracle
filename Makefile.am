modverdir = $(libdir)/$(mymodverdir)

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
	src/ql_oracle.qpp \
	test/db-test.q \
	test/sql-stmt.q \
	qore-oracle-module.spec

ACLOCAL_AMFLAGS=-I m4

if COND_DOXYGEN
DOX = oracle
DOXYGEN_OUTPUT = docs/oracle/html

DOXYFILES = docs/doxyfile docs/mainpage.doxygen
DOXYFILES_SRC = docs/doxyfile.tmpl docs/footer_template.html
QDX = qdx
DX_CLEANFILES = ${DOXYFILES}

docs/oracle/html: $(DOXYFILES)
	cd docs; $(DOXYGEN_CMD) doxyfile;
	$(QDX) --post $(DOXYGEN_OUTPUT)/*html
	$(QDX) --post $(DOXYGEN_OUTPUT)/search/*html

docs/mainpage.doxygen: docs/mainpage.doxygen.tmpl
	$(QDX) -d $< $@

docs/doxyfile: docs/doxyfile.tmpl
	sed s/__VERSION__/${VERSION}/ $< > $@

doxygen-doc: $(DOXYGEN_OUTPUT)

EXTRA_DIST += $(DOXYGEN_OUTPUT)

html-local: $(DOXYGEN_OUTPUT)

install-html-local:
	for dox in $(DOX); do $(MKDIR_P) $(DESTDIR)$(datadir)/$(PACKAGE)/docs/$$dox/html/search; done
	for dir in $(DOXYGEN_OUTPUT); do \
	   for f in $$dir/*; do if [ -f $$f ]; then $(INSTALL_DATA) $$f $(DESTDIR)$(datadir)/$(PACKAGE)/$$dir; fi; done; \
	   for f in $$dir/search/*; do $(INSTALL_DATA) $$f $(DESTDIR)$(datadir)/$(PACKAGE)/$$dir/search; done; \
	done

clean-local:
	$(RM) -rf $(DOXYGEN_OUTPUT)

uninstall-local:
	$(RM) -rf $(DESTDIR)$(datadir)/$(PACKAGE)/docs

mostlyclean-local:
	rm -rf ${DOXYGEN_OUTPUT} ${DX_CLEANFILES}

else
doxygen-doc:
	echo no doxygen support
endif

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
