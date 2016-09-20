/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  oracle.cpp

  Oracle OCI Interface to Qore DBI layer

  Qore Programming Language

  Copyright (C) 2003 - 2015 David Nichols

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "oracle.h"
#include "oracle-module.h"
#include "oracleobject.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <assert.h>

#include <memory>

DLLEXPORT char qore_module_name[] = "oracle";
DLLEXPORT char qore_module_version[] = PACKAGE_VERSION;
DLLEXPORT char qore_module_description[] = "Oracle database driver";
DLLEXPORT char qore_module_author[] = "David Nichols <david@qore.org>";
DLLEXPORT char qore_module_url[] = "http://qore.org";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = oracle_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = oracle_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = oracle_module_delete;
#ifdef _QORE_HAS_QL_MIT
DLLEXPORT qore_license_t qore_module_license = QL_MIT;
#else
DLLEXPORT qore_license_t qore_module_license = QL_LGPL;
#endif
DLLEXPORT char qore_module_license_str[] = "MIT";

void init_oracle_functions(QoreNamespace& ns);
QoreClass* initAQMessageClass(QoreNamespace& ns);
QoreClass* initAQQueueClass(QoreNamespace& ns);

DBIDriver *DBID_ORACLE = 0;

// capabilities of this driver
static int dbi_oracle_caps = (
   DBI_CAP_TRANSACTION_MANAGEMENT
   |DBI_CAP_STORED_PROCEDURES
   |DBI_CAP_CHARSET_SUPPORT
   |DBI_CAP_LOB_SUPPORT
   |DBI_CAP_BIND_BY_VALUE
   |DBI_CAP_BIND_BY_PLACEHOLDER
#ifdef _QORE_HAS_DBI_EXECRAW
   |DBI_CAP_HAS_EXECRAW
#endif
#ifdef _QORE_HAS_TIME_ZONES
   |DBI_CAP_TIME_ZONE_SUPPORT
#endif
#ifdef _QORE_HAS_NUMBER_TYPE
   |DBI_CAP_HAS_NUMBER_SUPPORT
#endif
#ifdef _QORE_HAS_FIND_CREATE_TIMEZONE
   |DBI_CAP_SERVER_TIME_ZONE
#endif
#ifdef DBI_CAP_AUTORECONNECT
   |DBI_CAP_AUTORECONNECT
#endif
#ifdef _QORE_HAS_DBI_EVENTS
   |DBI_CAP_EVENTS
#endif
#ifdef DBI_CAP_HAS_ARRAY_BIND
   |DBI_CAP_HAS_ARRAY_BIND
#endif
   );

static int oracle_commit(Datasource* ds, ExceptionSink* xsink) {
   QoreOracleConnection& conn = ds->getPrivateDataRef<QoreOracleConnection>();
   return conn.commit(xsink);
}

static int oracle_rollback(Datasource* ds, ExceptionSink* xsink) {
   QoreOracleConnection& conn = ds->getPrivateDataRef<QoreOracleConnection>();
   return conn.rollback(xsink);
}

static AbstractQoreNode* oracle_exec(Datasource* ds, const QoreString* qstr, const QoreListNode* args, ExceptionSink* xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.execWithPrologue(xsink, false);
}

static AbstractQoreNode* oracle_select(Datasource* ds, const QoreString* qstr, const QoreListNode* args, ExceptionSink* xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.execWithPrologue(xsink, false, true);
}

#ifdef _QORE_HAS_DBI_EXECRAW
static AbstractQoreNode* oracle_exec_raw(Datasource* ds, const QoreString* qstr, ExceptionSink* xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, 0, false, xsink))
      return 0;

   return bg.execWithPrologue(xsink, false);
}
#endif

#ifdef _QORE_HAS_DBI_EXECRAWROWS
static AbstractQoreNode* oracle_exec_raw_rows(Datasource* ds, const QoreString* qstr, ExceptionSink* xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, 0, false, xsink))
      return 0;

   return bg.execWithPrologue(xsink, true);
}
#endif

#ifdef _QORE_HAS_DBI_SELECT_ROW
static QoreHashNode* oracle_select_row(Datasource* ds, const QoreString* qstr, const QoreListNode* args, ExceptionSink* xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.selectRow(xsink);
}
#endif

static AbstractQoreNode* oracle_exec_rows(Datasource* ds, const QoreString* qstr, const QoreListNode* args, ExceptionSink* xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.execWithPrologue(xsink, true);
}

static int oracle_open(Datasource* ds, ExceptionSink* xsink) {
   //printd(5, "oracle_open() datasource %p for DB=%s open\n", ds, ds->getDBName());

   if (!ds->getUsername()) {
      xsink->raiseException("DATASOURCE-MISSING-USERNAME", "Datasource has an empty username parameter");
      return -1;
   }
   if (!ds->getPassword()) {
      xsink->raiseException("DATASOURCE-MISSING-PASSWORD", "Datasource has an empty password parameter");
      return -1;
   }
   if (!ds->getDBName()) {
      xsink->raiseException("DATASOURCE-MISSING-DBNAME", "Datasource has an empty dbname parameter");
      return -1;
   }

#ifdef QORE_HAS_DATASOURCE_PORT
   int port = ds->getPort();
#else
   int port = 0;
#endif

   if (port && !ds->getHostName()) {
      xsink->raiseException("DATASOURCE-MISSING-HOSTNAME", "port is set to %d, but no hostname is set; both hostname and port must be set to make a direct connection without TNS", port);
      return -1;
   }

   if (!port && ds->getHostName()) {
      xsink->raiseException("DATASOURCE-MISSING-PORT", "hostname is set to '%s', but no port is set; both hostname and port must be set to make a direct connection without TNS", ds->getHostName());
      return -1;
   }

   std::unique_ptr<QoreOracleConnection> conn(new QoreOracleConnection(*ds, xsink));
   if (*xsink)
      return -1;

   ds->setPrivateData((void *)conn.release());
   return 0;
}

static int oracle_close(Datasource* ds) {
   QORE_TRACE("oracle_close()");

   QoreOracleConnection *conn = (QoreOracleConnection *)ds->getPrivateData();

   delete conn;

   ds->setPrivateData(0);

   return 0;
}

static AbstractQoreNode* oracle_get_server_version(Datasource* ds, ExceptionSink* xsink) {
   // get private data structure for connection
   QoreOracleConnection& conn = ds->getPrivateDataRef<QoreOracleConnection>();
   return conn.getServerVersion(xsink);
}

#ifdef HAVE_OCICLIENTVERSION
static AbstractQoreNode* oracle_get_client_version(const Datasource* ds, ExceptionSink* xsink) {
   sword major, minor, update, patch, port_update;

   OCIClientVersion(&major, &minor, &update, &patch, &port_update);
   QoreHashNode* h = new QoreHashNode();
   h->setKeyValue("major", new QoreBigIntNode(major), NULL);
   h->setKeyValue("minor", new QoreBigIntNode(minor), NULL);
   h->setKeyValue("update", new QoreBigIntNode(update), NULL);
   h->setKeyValue("patch", new QoreBigIntNode(patch), NULL);
   h->setKeyValue("port_update", new QoreBigIntNode(port_update), NULL);
   return h;
}
#endif

#ifdef _QORE_HAS_PREPARED_STATMENT_API
static int oracle_stmt_prepare(SQLStatement* stmt, const QoreString &str, const QoreListNode* args, ExceptionSink* xsink) {
   assert(!stmt->getPrivateData());

   QorePreparedStatement* bg = new QorePreparedStatement(stmt->getDatasource());
   stmt->setPrivateData(bg);

   return bg->prepare(str, args, true, xsink);
}

static int oracle_stmt_prepare_raw(SQLStatement* stmt, const QoreString &str, ExceptionSink* xsink) {
   assert(!stmt->getPrivateData());

   QorePreparedStatement* bg = new QorePreparedStatement(stmt->getDatasource());
   stmt->setPrivateData(bg);

   return bg->prepare(str, 0, false, xsink);
}

static int oracle_stmt_bind(SQLStatement* stmt, const QoreListNode& l, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->bind(&l, xsink);
}

static int oracle_stmt_bind_placeholders(SQLStatement* stmt, const QoreListNode& l, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->bindPlaceholders(&l, xsink);
}

static int oracle_stmt_bind_values(SQLStatement* stmt, const QoreListNode& l, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->bindValues(&l, xsink);
}

static int oracle_stmt_exec(SQLStatement* stmt, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->exec(xsink);
}

static int oracle_stmt_define(SQLStatement* stmt, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->define(xsink);
}

static int oracle_stmt_affected_rows(SQLStatement* stmt, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->affectedRows(xsink);
}

static QoreHashNode* oracle_stmt_get_output(SQLStatement* stmt, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->getOutput(xsink);
}

static QoreHashNode* oracle_stmt_get_output_rows(SQLStatement* stmt, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->getOutputRows(xsink);
}

static QoreHashNode* oracle_stmt_fetch_row(SQLStatement* stmt, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->fetchRow(xsink);
}

static QoreListNode* oracle_stmt_fetch_rows(SQLStatement* stmt, int rows, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->fetchRows(rows, xsink);
}

static QoreHashNode* oracle_stmt_fetch_columns(SQLStatement* stmt, int rows, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->fetchColumns(rows, xsink);
}

#ifdef _QORE_HAS_DBI_DESCRIBE
static QoreHashNode* oracle_stmt_describe(SQLStatement* stmt, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->describe(xsink);
}
#endif

static bool oracle_stmt_next(SQLStatement* stmt, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   return bg->next(xsink);
}

static int oracle_stmt_close(SQLStatement* stmt, ExceptionSink* xsink) {
   QorePreparedStatement* bg = (QorePreparedStatement*)stmt->getPrivateData();
   assert(bg);

   bg->reset(xsink);
   delete bg;
   stmt->setPrivateData(0);
   return *xsink ? -1 : 0;
}
#endif // _QORE_HAS_PREPARED_STATMENT_API

#ifdef _QORE_HAS_DBI_OPTIONS
static int oracle_opt_set(Datasource* ds, const char* opt, const AbstractQoreNode* val, ExceptionSink* xsink) {
   // get private data structure for connection
   QoreOracleConnection& conn = ds->getPrivateDataRef<QoreOracleConnection>();
   return conn.setOption(opt, val, xsink);
}

static AbstractQoreNode* oracle_opt_get(const Datasource* ds, const char* opt) {
   // get private data structure for connection
   QoreOracleConnection& conn = ds->getPrivateDataRef<QoreOracleConnection>();
   return conn.getOption(opt);
}
#endif

QoreNamespace OraNS("Oracle");

QoreStringNode* oracle_module_init() {
   QORE_TRACE("oracle_module_init()");

   init_oracle_functions(OraNS);
   OraNS.addSystemClass(initAQMessageClass(OraNS));
   OraNS.addSystemClass(initAQQueueClass(OraNS));

   // register driver with DBI subsystem
   qore_dbi_method_list methods;
   methods.add(QDBI_METHOD_OPEN, oracle_open);
   methods.add(QDBI_METHOD_CLOSE, oracle_close);
   methods.add(QDBI_METHOD_SELECT, oracle_select);
   methods.add(QDBI_METHOD_SELECT_ROWS, oracle_exec_rows);
#ifdef _QORE_HAS_DBI_SELECT_ROW
   methods.add(QDBI_METHOD_SELECT_ROW, oracle_select_row);
#endif
   methods.add(QDBI_METHOD_EXEC, oracle_exec);
#ifdef _QORE_HAS_DBI_EXECRAW
   methods.add(QDBI_METHOD_EXECRAW, oracle_exec_raw);
#endif
#ifdef _QORE_HAS_DBI_EXECRAWROWS
   methods.add(QDBI_METHOD_EXECRAW, oracle_exec_raw_rows);
#endif
   methods.add(QDBI_METHOD_COMMIT, oracle_commit);
   methods.add(QDBI_METHOD_ROLLBACK, oracle_rollback);
   methods.add(QDBI_METHOD_GET_SERVER_VERSION, oracle_get_server_version);
#ifdef HAVE_OCICLIENTVERSION
   methods.add(QDBI_METHOD_GET_CLIENT_VERSION, oracle_get_client_version);
#endif

#ifdef _QORE_HAS_PREPARED_STATMENT_API
   methods.add(QDBI_METHOD_STMT_PREPARE, oracle_stmt_prepare);
   methods.add(QDBI_METHOD_STMT_PREPARE_RAW, oracle_stmt_prepare_raw);
   methods.add(QDBI_METHOD_STMT_BIND, oracle_stmt_bind);
   methods.add(QDBI_METHOD_STMT_BIND_PLACEHOLDERS, oracle_stmt_bind_placeholders);
   methods.add(QDBI_METHOD_STMT_BIND_VALUES, oracle_stmt_bind_values);
   methods.add(QDBI_METHOD_STMT_EXEC, oracle_stmt_exec);
   methods.add(QDBI_METHOD_STMT_DEFINE, oracle_stmt_define);
   methods.add(QDBI_METHOD_STMT_FETCH_ROW, oracle_stmt_fetch_row);
   methods.add(QDBI_METHOD_STMT_FETCH_ROWS, oracle_stmt_fetch_rows);
   methods.add(QDBI_METHOD_STMT_FETCH_COLUMNS, oracle_stmt_fetch_columns);
#ifdef _QORE_HAS_DBI_DESCRIBE
   methods.add(QDBI_METHOD_STMT_DESCRIBE, oracle_stmt_describe);
#endif
   methods.add(QDBI_METHOD_STMT_NEXT, oracle_stmt_next);
   methods.add(QDBI_METHOD_STMT_CLOSE, oracle_stmt_close);
   methods.add(QDBI_METHOD_STMT_AFFECTED_ROWS, oracle_stmt_affected_rows);
   methods.add(QDBI_METHOD_STMT_GET_OUTPUT, oracle_stmt_get_output);
   methods.add(QDBI_METHOD_STMT_GET_OUTPUT_ROWS, oracle_stmt_get_output_rows);
#endif // _QORE_HAS_PREPARED_STATMENT_API

#ifdef _QORE_HAS_DBI_OPTIONS
   methods.add(QDBI_METHOD_OPT_SET, oracle_opt_set);
   methods.add(QDBI_METHOD_OPT_GET, oracle_opt_get);

   methods.registerOption(DBI_OPT_NUMBER_OPT, "when set, number values are returned as integers if possible, otherwise as arbitrary-precision number values; the argument is ignored; setting this option turns it on and turns off 'string-numbers' and 'numeric-numbers'");
   methods.registerOption(DBI_OPT_NUMBER_STRING, "when set, number values are returned as strings for backwards-compatibility; the argument is ignored; setting this option turns it on and turns off 'optimal-numbers' and 'numeric-numbers'");
   methods.registerOption(DBI_OPT_NUMBER_NUMERIC, "when set, number values are returned as arbitrary-precision number values; the argument is ignored; setting this option turns it on and turns off 'string-numbers' and 'optimal-numbers'");
#endif
   methods.registerOption(DBI_OPT_TIMEZONE, "set the server-side timezone, value must be a string in the format accepted by Timezone::constructor() on the client (ie either a region name or a UTC offset like \"+01:00\"), if not set the server's time zone will be assumed to be the same as the client's", stringTypeInfo);

   DBID_ORACLE = DBI.registerDriver("oracle", methods, dbi_oracle_caps);

   return 0;
}

void oracle_module_ns_init(QoreNamespace* rns, QoreNamespace* qns) {
   QORE_TRACE("oracle_module_ns_init()");
   qns->addInitialNamespace(OraNS.copy());
}

void oracle_module_delete() {
   QORE_TRACE("oracle_module_delete()");
}
