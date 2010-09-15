/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  oracle.cpp

  Oracle OCI Interface to Qore DBI layer

  Qore Programming Language

  Copyright 2003 - 2010 David Nichols

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
#include "ocilib_checks.h"
#include "ocilib_defs.h"
#include "ocilib_internal.h"
#include "ocilib_types.h"
#include "oci_loader.h"
#include "oci_types.h"

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
DLLEXPORT char qore_module_author[] = "David Nichols";
DLLEXPORT char qore_module_url[] = "http://qore.org";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = oracle_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = oracle_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = oracle_module_delete;
DLLEXPORT qore_license_t qore_module_license = QL_LGPL;

DBIDriver *DBID_ORACLE = 0;

static int oracle_commit(Datasource *ds, ExceptionSink *xsink) {
   QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();
   return conn.commit(xsink);
}

static int oracle_rollback(Datasource *ds, ExceptionSink *xsink) {
   QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();
   return conn.rollback(xsink);
}

static AbstractQoreNode *oracle_exec(Datasource *ds, const QoreString *qstr, const QoreListNode *args, ExceptionSink *xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.execWithPrologue(false, xsink);
}

#ifdef _QORE_HAS_DBI_EXECRAW
static AbstractQoreNode *oracle_exec_raw(Datasource *ds, const QoreString *qstr, ExceptionSink *xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, 0, false, xsink))
      return 0;

   return bg.execWithPrologue(false, xsink);
}
#endif

static AbstractQoreNode *oracle_select(Datasource *ds, const QoreString *qstr, const QoreListNode *args, ExceptionSink *xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.select(xsink);
}

static AbstractQoreNode *oracle_select_rows(Datasource *ds, const QoreString *qstr, const QoreListNode *args, ExceptionSink *xsink) {
   QorePreparedStatementHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.selectRows(xsink);
}

static int oracle_open(Datasource *ds, ExceptionSink *xsink) {
//    printd(5, "oracle_open() datasource %p for DB=%s open\n", ds, ds->getDBName());

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

   QoreString db;
   if (port)
      db.sprintf("//%s:%d/%s", ds->getHostName(), port, ds->getDBName());
   else
      db.concat(ds->getDBName());

   //    printd(5, "oracle_open(): user=%s pass=%s db=%s (oracle encoding=%s)\n",
   // 	  ds->getUsername(), ds->getPassword(), db.getBuffer(), ds->getDBEncoding() ? ds->getDBEncoding() : "(none)");

   std::auto_ptr<QoreOracleConnection> conn(new QoreOracleConnection(*ds));

   // locking is done on the level above with the Datasource class
   int oci_flags = OCI_DEFAULT|OCI_THREADED|OCI_NO_MUTEX|OCI_OBJECT;

   const char *charset;

   // FIXME: maybe I don't need a temporary environment handle?
   // create temporary environment handle
   OCIEnv *tmpenvhp;
   OCIEnvCreate(&tmpenvhp, oci_flags | OCI_NO_UCB, 0, 0, 0, 0, 0, 0);
   // declare temporary buffer
   char nbuf[OCI_NLS_MAXBUFSZ];
   int need_to_set_charset = 0;

   if (ds->getDBEncoding()) {
      charset = ds->getDBEncoding();
      need_to_set_charset = 1;
   }
   else { // get Oracle character set name from OS character set name
      if ((OCINlsNameMap(tmpenvhp, (oratext *)nbuf, OCI_NLS_MAXBUFSZ, (oratext *)QCS_DEFAULT->getCode(), OCI_NLS_CS_IANA_TO_ORA) != OCI_SUCCESS)) {
	 OCIHandleFree(tmpenvhp, OCI_HTYPE_ENV);
	 xsink->raiseException("DBI:ORACLE:UNKNOWN-CHARACTER-SET", 
			       "cannot map default OS encoding '%s' to Oracle character encoding",
			       QCS_DEFAULT->getCode());
	 return -1;
      }
      ds->setDBEncoding(nbuf);
      ds->setQoreEncoding(QCS_DEFAULT);
      charset = nbuf;
//       printd(5, "oracle_open() setting Oracle encoding to '%s' from default OS encoding '%s'\n",
// 	     charset, QCS_DEFAULT->getCode());
   }

#ifndef HAVE_OCIENVNLSCREATE
#error need to define HAVE_OCIENVNLSCREATE (with Oracle 9i+)
#endif // HAVE_OCIENVNLSCREATE

   // get character set ID
   conn->charsetid = OCINlsCharSetNameToId(tmpenvhp, (oratext *)charset);
   // delete temporary environmental handle
   OCIHandleFree(tmpenvhp, OCI_HTYPE_ENV);

   if (!conn->charsetid) {
      xsink->raiseException("DBI:ORACLE:UNKNOWN-CHARACTER-SET", "this installation of Oracle does not support the '%s' character encoding", 
			    ds->getDBEncoding());
      return -1;
   }

//    printd(5, "Oracle character encoding '%s' has ID %d, oci_flags=%d\n", charset, conn->charsetid, oci_flags);
   // create environment with default character set
   if (OCIEnvNlsCreate(&conn->envhp, oci_flags, 0, NULL, NULL, NULL, 0, NULL, conn->charsetid, conn->charsetid) != OCI_SUCCESS) {
      xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "error creating new environment handle with encoding '%s'", ds->getDBEncoding());
      return -1;
   }

   //printd(0, "oracle_open() ds=%p allocated envhp=%p\n", ds, conn->envhp);

   // map the Oracle character set to a qore character set
   if (need_to_set_charset) {
      // map Oracle character encoding name to QORE/OS character encoding name
      if ((OCINlsNameMap(conn->envhp, (oratext *)nbuf, OCI_NLS_MAXBUFSZ, (oratext *)ds->getDBEncoding(), OCI_NLS_CS_ORA_TO_IANA) == OCI_SUCCESS)) {
// 	 printd(5, "oracle_open() Oracle character encoding '%s' mapped to '%s' character encoding\n", ds->getDBEncoding(), nbuf);
	 ds->setQoreEncoding(nbuf);
      }
      else {
	 xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "error mapping Oracle encoding '%s' to a qore encoding: unknown encoding", ds->getDBEncoding());
	 return -1;
      }
   }

   if (OCIHandleAlloc(conn->envhp, (dvoid **) &conn->errhp, OCI_HTYPE_ERROR, 0, 0) != OCI_SUCCESS) {
      OCIHandleFree(conn->envhp, OCI_HTYPE_ENV);
      xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "failed to allocate error handle for connection");
      return -1;
   }
   //printd(5, "oracle_open() about to call OCILogon()\n");
   conn->checkerr(OCILogon(conn->envhp, conn->errhp, &conn->svchp, (text *)ds->getUsername(), strlen(ds->getUsername()), (text *)ds->getPassword(), strlen(ds->getPassword()), (text *)db.getBuffer(), db.strlen()), 
                   "<open>", xsink);
   if (*xsink) {
      OCIHandleFree(conn->errhp, OCI_HTYPE_ERROR);
      OCIHandleFree(conn->envhp, OCI_HTYPE_ENV);
      return -1;
   }

//    printd(5, "oracle_open() datasource %p for DB=%s open (envhp=%p)\n", ds, db.getBuffer(), conn->envhp);
   
   if (!OCI_Initialize2(&conn->ocilib, conn->envhp, conn->errhp, ocilib_err_handler, NULL, oci_flags)) {
       xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "failed to allocate OCILIB support handlers");
       return -1;
   }

   //printd(5, "conn->ocilib=%p mode=%d\n", &conn->ocilib, conn->ocilib.env_mode);
   
   conn->ocilib_cn = new OCI_Connection;
   // fake the OCI_Connection
   conn->ocilib_cn->err = conn->errhp;
   conn->ocilib_cn->cxt = conn->svchp;
   conn->ocilib_cn->tinfs = OCI_ListCreate2(&conn->ocilib, OCI_IPC_TYPE_INFO);
   // then reset unused attributes
   conn->ocilib_cn->db = 0;
   conn->ocilib_cn->user = 0;      /* user */
   conn->ocilib_cn->pwd = 0;       /* password */
   conn->ocilib_cn->stmts = 0;     /* list of statements */
   conn->ocilib_cn->trsns = 0;     /* list of transactions */

   conn->ocilib_cn->trs=0;       /* pointer to current transaction object */
   conn->ocilib_cn->pool=0;      /* pointer to connection pool parent */
   conn->ocilib_cn->svopt=0;     /* Pointer to server output object */
   conn->ocilib_cn->svr=0;       /* OCI server handle */

   /* OCI session handle */
//    conn->checkerr(
//                 OCI_HandleAlloc((dvoid *) OCILib.env,
//                                                   (dvoid **) (void *) &conn->ocilib_cn->ses,
//                                                   (ub4) OCI_HTYPE_SESSION,
//                                                   (size_t) 0, (dvoid **) NULL),
//                 "oracle_open OCI_HTYPE_SESSION", xsink);
   conn->ocilib_cn->ses=0;       /* OCI session handle */
   conn->ocilib_cn->autocom=false;   /* auto commit mode */
   conn->ocilib_cn->nb_files=0;  /* number of OCI_File opened by the connection */
   conn->ocilib_cn->mode=0;      /* session mode */
   conn->ocilib_cn->cstate = 0;    /* connection state */
   conn->ocilib_cn->usrdata=0;   /* user data */

   conn->ocilib_cn->fmt_date = 0;  /* date string format for conversion */
   conn->ocilib_cn->fmt_num = 0;   /* numeric string format for conversion */
   conn->ocilib_cn->ver_str=0;   /* string  server version*/
   conn->ocilib_cn->ver_num = conn->ocilib.version_runtime;   /* numeric server version */
   conn->ocilib_cn->trace=0;     /* trace information */

   ds->setPrivateData((void *)conn.release());

   return 0;
}

static int oracle_close(Datasource *ds) {
   QORE_TRACE("oracle_close()");

   QoreOracleConnection *conn = (QoreOracleConnection *)ds->getPrivateData();

   //printd(0, "oracle_close() ds=%p envhp=%p ocilib envhp=%p\n", ds, conn->envhp, OCILib.env);

//    printd(0, "oracle_close(): connection to %s closed.\n", ds->getDBName());
//    printd(0, "oracle_close(): ptr: %p\n", conn);
//    printd(0, "oracle_close(): conn->svchp, conn->errhp: %p, %p\n", conn->svchp, conn->errhp);
   OCILogoff(conn->svchp, conn->errhp);
   OCIHandleFree(conn->svchp, OCI_HTYPE_SVCCTX);
//    OCIHandleFree(conn->errhp, OCI_HTYPE_ERROR); // deleted in OCI_Cleanup
//    OCIHandleFree(conn->envhp, OCI_HTYPE_ENV); // OCI_Cleanup
   OCI_ListForEach(&conn->ocilib, conn->ocilib_cn->tinfs, (boolean (*)(void *)) OCI_TypeInfoClose);
   OCI_ListFree(&conn->ocilib, conn->ocilib_cn->tinfs);

   OCI_Cleanup2(&conn->ocilib);

   delete conn;

   ds->setPrivateData(0);

   return 0;
}

#define VERSION_BUF_SIZE 512
static AbstractQoreNode *oracle_get_server_version(Datasource *ds, ExceptionSink *xsink) {
   // get private data structure for connection
   QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();
   
   // buffer for version information
   char version_buf[VERSION_BUF_SIZE + 1];
   
   // execute OCIServerVersion and check status code
   if (conn.checkerr(OCIServerVersion(conn.svchp, conn.errhp, (OraText *)version_buf, VERSION_BUF_SIZE, OCI_HTYPE_SVCCTX),
                     "oracle_get_server_version", xsink))      
      return 0;
   
   return new QoreStringNode(version_buf);   
}

#ifdef HAVE_OCICLIENTVERSION
static AbstractQoreNode *oracle_get_client_version(const Datasource *ds, ExceptionSink *xsink) {
   sword major, minor, update, patch, port_update;

   OCIClientVersion(&major, &minor, &update, &patch, &port_update);
   QoreHashNode *h = new QoreHashNode();
   h->setKeyValue("major", new QoreBigIntNode(major), NULL);
   h->setKeyValue("minor", new QoreBigIntNode(minor), NULL);
   h->setKeyValue("update", new QoreBigIntNode(update), NULL);
   h->setKeyValue("patch", new QoreBigIntNode(patch), NULL);
   h->setKeyValue("port_update", new QoreBigIntNode(port_update), NULL);
   return h;
}
#endif

#ifdef _QORE_HAS_PREPARED_STATMENT_API
static int oracle_stmt_prepare(SQLStatement *stmt, const QoreString &str, const QoreListNode *args, ExceptionSink *xsink) {
   assert(!stmt->getPrivateData());

   QorePreparedStatement *bg = new QorePreparedStatement(stmt->getDatasource());
   stmt->setPrivateData(bg);

   return bg->prepare(str, args, true, xsink);
}

static int oracle_stmt_prepare_raw(SQLStatement *stmt, const QoreString &str, ExceptionSink *xsink) {
   assert(!stmt->getPrivateData());

   QorePreparedStatement *bg = new QorePreparedStatement(stmt->getDatasource());
   stmt->setPrivateData(bg);

   return bg->prepare(str, 0, false, xsink);
}

static int oracle_stmt_bind(SQLStatement *stmt, const QoreListNode &l, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->bind(&l, xsink);
}

static int oracle_stmt_bind_placeholders(SQLStatement *stmt, const QoreListNode &l, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->bindPlaceholders(&l, xsink);
}

static int oracle_stmt_bind_values(SQLStatement *stmt, const QoreListNode &l, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->bindValues(&l, xsink);
}

static int oracle_stmt_exec(SQLStatement *stmt, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->exec(xsink);
}

static int oracle_stmt_define(SQLStatement *stmt, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->define(xsink);
}

static int oracle_stmt_affected_rows(SQLStatement *stmt, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->affectedRows(xsink);
}

static QoreHashNode *oracle_stmt_get_output(SQLStatement *stmt, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->getOutput(xsink);
}

static QoreHashNode *oracle_stmt_get_output_rows(SQLStatement *stmt, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->getOutputRows(xsink);
}

static QoreHashNode *oracle_stmt_fetch_row(SQLStatement *stmt, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->fetchRow(xsink);
}

static QoreListNode *oracle_stmt_fetch_rows(SQLStatement *stmt, int rows, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->fetchRows(rows, xsink);
}

static QoreHashNode *oracle_stmt_fetch_columns(SQLStatement *stmt, int rows, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->fetchColumns(rows, xsink);
}

static bool oracle_stmt_next(SQLStatement *stmt, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   return bg->next(xsink);
}

static int oracle_stmt_close(SQLStatement *stmt, ExceptionSink *xsink) {
   QorePreparedStatement *bg = (QorePreparedStatement *)stmt->getPrivateData();
   assert(bg);

   bg->reset(xsink);
   delete bg;
   stmt->setPrivateData(0);
   return *xsink ? -1 : 0;
}
#endif // _QORE_HAS_PREPARED_STATMENT_API

QoreStringNode *oracle_module_init() {
   QORE_TRACE("oracle_module_init()");
   
   builtinFunctions.add2("bindOracleObject", f_oracle_object, QC_NO_FLAGS, QDOM_DATABASE, hashTypeInfo, 2, stringTypeInfo, QORE_PARAM_NO_ARG, hashTypeInfo, QORE_PARAM_NO_ARG);
   builtinFunctions.add2("placeholderOracleObject", f_oracle_object_placeholder, QC_NO_FLAGS, QDOM_DATABASE, hashTypeInfo, 1, stringTypeInfo, QORE_PARAM_NO_ARG);
   builtinFunctions.add2("bindOracleCollection", f_oracle_collection, QC_NO_FLAGS, QDOM_DATABASE, hashTypeInfo, 2, stringTypeInfo, QORE_PARAM_NO_ARG, listTypeInfo, QORE_PARAM_NO_ARG);
   builtinFunctions.add2("placeholderOracleCollection", f_oracle_collection_placeholder, QC_NO_FLAGS, QDOM_DATABASE, hashTypeInfo, 1, stringTypeInfo, QORE_PARAM_NO_ARG);

   // register driver with DBI subsystem
   qore_dbi_method_list methods;
   methods.add(QDBI_METHOD_OPEN, oracle_open);
   methods.add(QDBI_METHOD_CLOSE, oracle_close);
   methods.add(QDBI_METHOD_SELECT, oracle_select);
   methods.add(QDBI_METHOD_SELECT_ROWS, oracle_select_rows);
   methods.add(QDBI_METHOD_EXEC, oracle_exec);
#ifdef _QORE_HAS_DBI_EXECRAW
   methods.add(QDBI_METHOD_EXECRAW, oracle_exec_raw);
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
   methods.add(QDBI_METHOD_STMT_NEXT, oracle_stmt_next);
   methods.add(QDBI_METHOD_STMT_CLOSE, oracle_stmt_close);
   methods.add(QDBI_METHOD_STMT_AFFECTED_ROWS, oracle_stmt_affected_rows);
   methods.add(QDBI_METHOD_STMT_GET_OUTPUT, oracle_stmt_get_output);
   methods.add(QDBI_METHOD_STMT_GET_OUTPUT_ROWS, oracle_stmt_get_output_rows);
#endif // _QORE_HAS_PREPARED_STATMENT_API
   
   DBID_ORACLE = DBI.registerDriver("oracle", methods, DBI_ORACLE_CAPS);

   return 0;
}

void oracle_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   QORE_TRACE("oracle_module_ns_init()");
}

void oracle_module_delete() {
   QORE_TRACE("oracle_module_delete()");
}
