ORACLE_SOURCES = oracle.cpp \
	oracleobject.cpp \
	QoreOracleConnection.cpp \
	QoreOracleStatement.cpp \
	QorePreparedStatement.cpp \
	OraColumnValue.cpp \
	OraResultSet.cpp \
	ocilib/exception.cpp \
	ocilib/string.cpp \
	ocilib/timestamp.cpp \
	ocilib/collection.cpp \
	ocilib/element.cpp \
	ocilib/library.cpp \
	ocilib/number.cpp \
	ocilib/typeinfo.cpp \
	ocilib/column.cpp \
	ocilib/error.cpp \
	ocilib/list.cpp \
	ocilib/memory.cpp \
	ocilib/lob.cpp \
	ocilib/object.cpp \
	ocilib/interval.cpp \
	ocilib/date.cpp \
	ocilib/threadkey.cpp \
	ocilib/mutex.cpp \
	ocilib/ref.cpp \
	ocilib/file.cpp \
	ocilib/connection.cpp

#	ocilib/long.cpp


lib_LTLIBRARIES = oracle.la
oracle_la_SOURCES = $(ORACLE_SOURCES)
oracle_la_LDFLAGS = -module -avoid-version ${ORACLE_LIBS}

INCLUDES = -I$(top_srcdir)/include $(ORACLE_INCLUDES) -I$(top_srcdir)/src/ocilib

AM_CPPFLAGS = ${QORE_CPPFLAGS}

if COND_PROFILE
AM_CXXFLAGS=-g -pg
else
if COND_DEBUG
AM_CXXFLAGS=-g
AM_CFLAGS=-g
endif
endif

install-exec-hook:
	mv $(DESTDIR)$(libdir)/oracle.$(MODULE_SUFFIX) $(DESTDIR)$(libdir)/oracle-api-$(MODULE_API).qmod
	rm -f $(DESTDIR)$(libdir)/oracle.la $(libdir)/oracle.a
