/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  QoreOracleConnection.h

  Qore Programming Language

  Copyright (C) 2003 - 2012 David Nichols

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

#include "ocilib_internal.h"

QoreOracleConnection::QoreOracleConnection(Datasource &n_ds, ExceptionSink *xsink) : errhp(0), svchp(0), ocilib_cn(0), ds(n_ds), ocilib_init(false), number_support(OPT_NUM_DEFAULT) {
   // locking is done on the level above with the Datasource class

#ifdef QORE_HAS_DATASOURCE_PORT
   int port = ds.getPort();
#else
   int port = 0;
#endif

   if (port)
      cstr.sprintf("//%s:%d/%s", ds.getHostName(), port, ds.getDBName());
   else
      cstr.concat(ds.getDBName());

   //printd(5, "QoreOracleConnection::QoreOracleConnection(): user=%s pass=%s db=%s (oracle encoding=%s)\n", ds.getUsername(), ds.getPassword(), db.getBuffer(), ds.getDBEncoding() ? ds.getDBEncoding() : "(none)");

   bool set_charset = false;

   QoreString encoding;

   {
      // create temporary environment handle
      QoreOracleEnvironment tmpenv;
      tmpenv.init();
      
      if (ds.getDBEncoding()) {
         set_charset = true;

         // get character set ID
         charsetid = tmpenv.nlsCharSetNameToId(ds.getDBEncoding());
      }
      else { // get Oracle character set name from OS character set name
         if (tmpenv.nlsNameMapToOracle(QCS_DEFAULT->getCode(), encoding) != OCI_SUCCESS) {
            xsink->raiseException("DBI:ORACLE:UNKNOWN-CHARACTER-ENCODING", "cannot map default OS encoding '%s' to Oracle character encoding", QCS_DEFAULT->getCode());
            return;
         }
         ds.setDBEncoding(encoding.getBuffer());
         ds.setQoreEncoding(QCS_DEFAULT);

         // get character set ID
         charsetid = tmpenv.nlsCharSetNameToId(encoding.getBuffer());
         // printd(5, "QoreOracleConnection::QoreOracleConnection() setting Oracle encoding to '%s' from default OS encoding '%s'\n", charset, QCS_DEFAULT->getCode());
      }
   }

   if (!charsetid) {
      xsink->raiseException("DBI:ORACLE:UNKNOWN-CHARACTER-ENCODING", "this installation of Oracle does not support the '%s' character encoding", ds.getDBEncoding());
      return;
   }

   // printd(5, "Oracle character encoding '%s' has ID %d, OCI_FLAGS=%d\n", charset, charsetid, OCI_FLAGS);
   // create environment with default character set
   if (env.init(charsetid)) {
      xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "error creating new environment handle with encoding '%s'", ds.getDBEncoding());
      return;
   }

   //printd(0, "QoreOracleConnection::QoreOracleConnection() ds=%p allocated envhp=%p\n", ds, *env);

   // map the Oracle character set to a qore character set
   if (set_charset) {
      // map Oracle character encoding name to QORE/OS character encoding name
      if (!env.nlsNameMapToQore(ds.getDBEncoding(), encoding)) {
         //printd(0, "QoreOracleConnection::QoreOracleConnection() Oracle character encoding '%s' mapped to '%s' character encoding\n", ds.getDBEncoding(), encoding.getBuffer());
         assert(encoding.strlen());
	 ds.setQoreEncoding(encoding.getBuffer());
      }
      else {
	 xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "error mapping Oracle character encoding '%s' to a qore encoding: unknown encoding", ds.getDBEncoding());
	 return;
      }
   }

   // cannot use handleAlloc() here as we are allocating errhp now
   if (OCIHandleAlloc(*env, (dvoid **) &errhp, OCI_HTYPE_ERROR, 0, 0) != OCI_SUCCESS) {
      xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "failed to allocate error handle for connection");
      return;
   }

   //printd(5, "QoreOracleConnection::QoreOracleConnection() about to call OCILogon()\n");
   if (logon(xsink))
      return;
   if (checkWarnings(xsink))
      return;

   //printd(5, "QoreOracleConnection::QoreOracleConnection() datasource %p for DB=%s open (envhp=%p)\n", ds, cstr.getBuffer(), *env);
   
   if (!OCI_Initialize2(&ocilib, *env, errhp, ocilib_err_handler, 0, QORE_OCI_FLAGS)) {
      xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "failed to allocate OCILIB support handlers");
      return;
   }

   ocilib_init = true;

   //printd(5, "ocilib=%p mode=%d\n", &ocilib, ocilib.env_mode);
   
   ocilib_cn = new OCI_Connection;
   // fake the OCI_Connection
   ocilib_cn->err = errhp;
   ocilib_cn->cxt = svchp;
   ocilib_cn->tinfs = OCI_ListCreate2(&ocilib, OCI_IPC_TYPE_INFO);

   // then reset unused attributes
   ocilib_cn->db = 0;
   ocilib_cn->user = 0;                           // user
   ocilib_cn->pwd = 0;                            // password
   ocilib_cn->stmts = 0;                          // list of statements
   ocilib_cn->trsns = 0;                          // list of transactions

   ocilib_cn->trs = 0;                            // pointer to current transaction object
   ocilib_cn->pool = 0;                           // pointer to connection pool parent
   ocilib_cn->svopt = 0;                          // Pointer to server output object
   ocilib_cn->svr = 0;                            // OCI server handle

   ocilib_cn->ses = 0;                            // OCI session handle
   ocilib_cn->autocom = false;                    // auto commit mode
   ocilib_cn->nb_files = 0;                       // number of OCI_File opened by the connection
   ocilib_cn->mode = 0;                           // session mode
   ocilib_cn->cstate = 0;                         // connection state
   ocilib_cn->usrdata = 0;                        // user data

   ocilib_cn->fmt_date = 0;                       // date string format for conversion
   ocilib_cn->fmt_num = 0;                        // numeric string format for conversion
   ocilib_cn->ver_str = 0;                        // string  server version
   ocilib_cn->ver_num = ocilib.version_runtime;   // numeric server version
   ocilib_cn->trace = 0;                          // trace information   

   //printd(0, "QoreOracleConnection::QoreOracleConnection() this=%p ds=%p envhp=%p svchp=%p errhp=%p\n", this, &ds, *env, svchp, errhp);
}

QoreOracleConnection::~QoreOracleConnection() {
   //printd(0, "QoreOracleConnection::~QoreOracleConnection() this=%p ds=%p envhp=%p svchp=%p ocilib envhp=%p\n", this, &ds, *env, svchp, ocilib.env);
   //printd(0, "QoreOracleConnection::~QoreOracleConnection(): connection to %s closed.\n", ds.getDBName());
   //printd(0, "QoreOracleConnection::~QoreOracleConnection(): svchp, errhp: %p, %p\n", svchp, errhp);
   if (svchp)
      logoff();

   if (ocilib_cn) {
      OCI_ListForEach(&ocilib, ocilib_cn->tinfs, (boolean (*)(void *)) OCI_TypeInfoClose);
      OCI_ListFree(&ocilib, ocilib_cn->tinfs);

      OCI_FREE(ocilib_cn->fmt_num);

      delete ocilib_cn;
   }

   if (ocilib_init)
      OCI_Cleanup2(&ocilib);

   if (errhp)
      OCIHandleFree(errhp, OCI_HTYPE_ERROR);
}

int QoreOracleConnection::checkerr(sword status, const char *query_name, ExceptionSink *xsink) {
   sb4 errcode = 0;

   //printd(5, "QoreOracleConnection::checkerr(%p, %d, %s, isEvent=%d)\n", errhp, status, query_name ? query_name : "none", *xsink);
   switch (status) {
      case OCI_SUCCESS:
	 return 0;
      case OCI_SUCCESS_WITH_INFO: {
	 text errbuf[512];

	 OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *) NULL, &errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
         //printd(0, "WARNING: %s returned OCI_SUCCESS_WITH_INFO: %s\n", query_name ? query_name : "<unknown>", remove_trailing_newlines((char *)errbuf));
	 // ignore SUCCESS_WITH_INFO codes
	 return 0;
      }

      case OCI_ERROR: {
	 text errbuf[512];
	 OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *) NULL, &errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
	 if (query_name)
	    xsink->raiseException("DBI:ORACLE:OCI-ERROR", "%s@%s: %s: %s", ds.getUsername(), ds.getDBName(), query_name, remove_trailing_newlines((char *)errbuf));
	 else
	    xsink->raiseException("DBI:ORACLE:OCI-ERROR", "%s@%s: %s", ds.getUsername(), ds.getDBName(), remove_trailing_newlines((char *)errbuf));
	 break;
      }
      case OCI_INVALID_HANDLE:
	 if (query_name)
	    xsink->raiseException("DBI:ORACLE:OCI-INVALID-HANDLE", "%s@%s: %s: an invalid OCI handle was used", ds.getUsername(), ds.getDBName(), query_name);
	 else
	    xsink->raiseException("DBI:ORACLE:OCI-INVALID-HANDLE", "%s@%s: an invalid OCI handle was used", ds.getUsername(), ds.getDBName());
	 break;
      case OCI_NEED_DATA:
	 xsink->raiseException("DBI:ORACLE:OCI-NEED-DATA", "Oracle OCI error");
	 break;
      case OCI_NO_DATA:
	 xsink->raiseException("DBI:ORACLE:OCI-NODATA", "Oracle OCI error");
	 break;
      case OCI_STILL_EXECUTING:
	 xsink->raiseException("DBI:ORACLE:OCI-STILL-EXECUTING", "Oracle OCI error");
	 break;
      case OCI_CONTINUE:
	 xsink->raiseException("DBI:ORACLE:OCI-CONTINUE", "Oracle OCI error");
	 break;
      default:
	 xsink->raiseException("DBI:ORACLE:UNKNOWN-ERROR", "unknown OCI error code %d", status);
	 break;
   }
   return -1;
}

int QoreOracleConnection::descriptorAlloc(void **descpp, unsigned type, const char *who, ExceptionSink *xsink) {
   return checkerr(OCIDescriptorAlloc(*env, descpp, type, 0, 0), who, xsink);
}

int QoreOracleConnection::handleAlloc(void **hndlpp, unsigned type, const char *who, ExceptionSink *xsink) {
   return checkerr(OCIHandleAlloc(*env, hndlpp, type, 0, 0), who, xsink);
}

int QoreOracleConnection::commit(ExceptionSink *xsink) {
   return checkerr(OCITransCommit(svchp, errhp, (ub4) 0), "QoreOracleConnection:commit()", xsink);
}

int QoreOracleConnection::rollback(ExceptionSink *xsink) {
   return checkerr(OCITransRollback(svchp, errhp, (ub4) 0), "QoreOracleConnection:rollback()", xsink);
}

DateTimeNode *QoreOracleConnection::getTimestamp(bool get_tz, OCIDateTime *odt, ExceptionSink *xsink) {
   //printd(5, "QoreOracleConnection::getTimestamp() using TIMESTAMP handle %p\n", odt);
   sb2 year;
   ub1 month, day;
   if (checkerr(OCIDateTimeGetDate(*env, errhp, odt, &year, &month, &day), "OCIDateTimeGetDate()", xsink))
      return 0;

   ub1 hour, minute, second;
   ub4 ns; // nanoseconds
   if (checkerr(OCIDateTimeGetTime(*env, errhp, odt, &hour, &minute, &second, &ns), "OCIDateTimeGetTime()", xsink))
      return 0;

#ifdef _QORE_HAS_TIME_ZONES
   const AbstractQoreZoneInfo *zone;
   if (!get_tz)
      zone = currentTZ();
   else {
      // try to get time zone from date value
      // time zone offset, hour and minute
      sb1 oh = 0, om = 0;
      sword err = OCIDateTimeGetTimeZoneOffset(*env, errhp, odt, &oh, &om);
      if (err == OCI_SUCCESS) {
	 //printd(5, "err=%d, oh=%d, om=%d, se=%d\n", err, (int)oh, (int)om, oh * 3600 + om * 60);
	 zone = findCreateOffsetZone(oh * 3600 + om * 60);
      }
      else {
	 //printd(5, "QoreOracleConnection::getTimestamp() this=%p time zone retrieval failed (%04d-%02d-%02d %02d:%02d:%02d)\n", this, year, month, day, hour, minute, second);
 	 // no time zone info, assume local time
	 zone = currentTZ();
      }
   }
   return DateTimeNode::makeAbsolute(zone, year, month, day, hour, minute, second, ns / 1000);
#else
   return new DateTimeNode(year, month, day, hour, minute, second, ns / 1000000);
#endif
}

BinaryNode *QoreOracleConnection::readBlob(OCILobLocator *lobp, ExceptionSink *xsink) {
   // retrieve *LOB data
   void *dbuf = malloc(LOB_BLOCK_SIZE);
   ON_BLOCK_EXIT(free, dbuf);
   ub4 amt = 0;

   SimpleRefHolder<BinaryNode> b(new BinaryNode);
   // read LOB data in streaming callback mode
   if (checkerr(OCILobRead(svchp, errhp, lobp, &amt, 1, dbuf, LOB_BLOCK_SIZE, *b, readBlobCallback, 0, 0), "QoreOracleConnection::readClob()", xsink))
      return 0;
   return b.release();
}

QoreStringNode *QoreOracleConnection::readClob(OCILobLocator *lobp, const QoreEncoding *enc, ExceptionSink *xsink) {
   void *dbuf = malloc(LOB_BLOCK_SIZE);
   ON_BLOCK_EXIT(free, dbuf);
   ub4 amt = 0;

   QoreStringNodeHolder str(new QoreStringNode(enc));
   // read LOB data in streaming callback mode
   if (checkerr(OCILobRead(svchp, errhp, lobp, &amt, 1, dbuf, LOB_BLOCK_SIZE, *str, readClobCallback, (ub2)charsetid, 0), "QoreOracleConnection::readBlob()", xsink))
      return 0;
   return str.release();
}
