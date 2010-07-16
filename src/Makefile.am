ORACLE_SOURCES = oracle.cc oracleobject.cpp \
	ocilib/array.c \
	ocilib/bind.c \
	ocilib/callback.c \
	ocilib/connection.c \
	ocilib/define.c \
	ocilib/exception.c \
	ocilib/handle.c \
	ocilib/iterator.c \
	ocilib/lob.c \
	ocilib/mutex.c \
	ocilib/resultset.c \
	ocilib/string.c \
	ocilib/timestamp.c \
	ocilib/collection.c \
	ocilib/connpool.c \
	ocilib/element.c \
	ocilib/file.c \
	ocilib/hash.c \
	ocilib/library.c \
	ocilib/long.c \
	ocilib/number.c \
	ocilib/typeinfo.c \
	ocilib/thread.c \
	ocilib/transaction.c \
	ocilib/column.c \
	ocilib/date.c \
	ocilib/error.c \
	ocilib/format.c \
	ocilib/interval.c \
	ocilib/list.c \
	ocilib/memory.c \
	ocilib/object.c \
	ocilib/statement.c \
	ocilib/ref.c \
	ocilib/threadkey.c \
	ocilib/dirpath.c \
	ocilib/event.c \
	ocilib/subscription.c

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
endif
endif

install-exec-hook:
	mv $(DESTDIR)$(libdir)/oracle.$(MODULE_SUFFIX) $(DESTDIR)$(libdir)/oracle-api-$(MODULE_API).qmod
	rm -f $(DESTDIR)$(libdir)/oracle.la $(libdir)/oracle.a
