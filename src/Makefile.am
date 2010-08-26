ORACLE_SOURCES = oracle.cc oracleobject.cpp \
	ocilib/exception.c \
	ocilib/string.c \
	ocilib/timestamp.c \
	ocilib/collection.c \
	ocilib/element.c \
	ocilib/library.c \
	ocilib/number.c \
	ocilib/typeinfo.c \
	ocilib/column.c \
	ocilib/error.c \
	ocilib/list.c \
	ocilib/memory.c \
	ocilib/lob.c \
	ocilib/object.c \
	ocilib/interval.c \
	ocilib/date.c \
	ocilib/threadkey.c \
	ocilib/mutex.c

#	ocilib/hash.c 
#	ocilib/long.c


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
