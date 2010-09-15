/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  QoreOracleStatement.h

  Qore Programming Language

  Copyright (C) 2003 - 2010 David Nichols

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

#ifndef _QORE_QOREORACLESTATEMENT_H

#define _QORE_QOREORACLESTATEMENT_H

#include "../config.h"
#include "oracle-config.h"
#include <qore/Qore.h>

#include <oci.h>

class OraResultSet;

class QoreOracleStatement {
protected:
   Datasource *ds;
   OCIStmt *stmthp;
   bool is_select;

   DLLLOCAL void del() {
      // free OCI handle
      OCIHandleFree(stmthp, OCI_HTYPE_STMT);
   }

public:
   DLLLOCAL QoreOracleStatement(Datasource *n_ds, OCIStmt *n_stmthp = 0) : ds(n_ds), stmthp(n_stmthp), is_select(false) {
   }

   DLLLOCAL ~QoreOracleStatement() {
      if (stmthp)
         del();
   }

   DLLLOCAL operator bool() const {
      return stmthp;
   }

   DLLLOCAL void reset(ExceptionSink *xsink) {
      // free OCI handle
      if (stmthp) {
         del();
         stmthp = 0;
      }

      is_select = false;
   }

   DLLLOCAL int allocate(ExceptionSink *xsink) {
      assert(!stmthp);

      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();
      
      if (conn.handleAlloc((dvoid **)&stmthp, OCI_HTYPE_STMT, "QoreOracleStatement::allocate()", xsink)) {
         stmthp = 0;
         return -1;
      }

      return 0;
   }

   // returns 0=OK, -1=ERROR
   DLLLOCAL int paramGet(OCIParam *&parmp, unsigned pos) {
      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();
      return OCIParamGet(stmthp, OCI_HTYPE_STMT, conn.errhp, (void**)&parmp, pos) == OCI_SUCCESS ? 0 : -1;
   }

   DLLLOCAL int attrGet(OCIParam *parmp, void *attributep, unsigned &sizep, unsigned attrtype, ExceptionSink *xsink) {
      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();
      return conn.checkerr(OCIAttrGet(parmp, OCI_DTYPE_PARAM, attributep, &sizep, attrtype, conn.errhp), "QoreOracleStatement::attrGet():param1", xsink);
   }

   DLLLOCAL int attrGet(OCIParam *parmp, void *attributep, unsigned attrtype, ExceptionSink *xsink) {
      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();
      return conn.checkerr(OCIAttrGet(parmp, OCI_DTYPE_PARAM, attributep, 0, attrtype, conn.errhp), "QoreOracleStatement::attrGet():param2", xsink);
   }

   DLLLOCAL int attrGet(void *attributep, unsigned &sizep, unsigned attrtype, ExceptionSink *xsink) {
      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();
      return conn.checkerr(OCIAttrGet(stmthp, OCI_HTYPE_STMT, attributep, &sizep, attrtype, conn.errhp), "QoreOracleStatement::attrGet():stmt1", xsink);
   }

   DLLLOCAL int attrGet(void *attributep, unsigned attrtype, ExceptionSink *xsink) {
      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();
      return conn.checkerr(OCIAttrGet(stmthp, OCI_HTYPE_STMT, attributep, 0, attrtype, conn.errhp), "QoreOracleStatement::attrGet():stmt2", xsink);
   }

   // returns 0=OK, -1=ERROR (exception), 1=no data
   DLLLOCAL int fetch(ExceptionSink *xsink, unsigned rows = 1) {
      int status;

      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();

      if ((status = OCIStmtFetch2(stmthp, conn.errhp, rows, OCI_FETCH_NEXT, 0, OCI_DEFAULT))) {
	 if (status == OCI_NO_DATA)
	    return 1;
	 else if (conn.checkerr(status, "QoreOracleStatement::fetch()", xsink))
            return -1;
      }
      return 0;
   }

   DLLLOCAL int defineByPos(OCIDefine *&defp, unsigned pos, void *valuep, int value_sz, unsigned short dty, void *indp, ExceptionSink *xsink) {
      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();

      return conn.checkerr(OCIDefineByPos(stmthp, &defp, conn.errhp, pos, valuep, value_sz, dty, indp, 0, 0, OCI_DEFAULT), 
			     "QoreOracleStatement::defineByPos()", xsink);
   }

   DLLLOCAL int bindByPos(OCIBind *&bndp, unsigned pos, void *valuep, int value_sz, unsigned short dty, ExceptionSink *xsink, void *indp = 0) {
      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();

      return conn.checkerr(OCIBindByPos(stmthp, &bndp, conn.errhp, pos, valuep, value_sz, dty, indp, 0, 0, 0, 0, OCI_DEFAULT), 
			     "QoreOracleStatement::bindByPos()", xsink);
   }


   DLLLOCAL int prepare(QoreString &str, ExceptionSink *xsink) {
      QoreOracleConnection &conn = ds->getPrivateDataRef<QoreOracleConnection>();

      int rc = conn.checkerr(OCIStmtPrepare(stmthp, conn.errhp, (text *)str.getBuffer(), str.strlen(), OCI_NTV_SYNTAX, OCI_DEFAULT), 
			     "QoreOracleStatement::prepare()", xsink);
      if (!rc) {
	 // see what kind of statement was prepared and set the flag accordingly
	 ub2 stype;
	 if (attrGet(&stype, OCI_ATTR_STMT_TYPE, xsink))
	    return -1;

	 if (stype == OCI_STMT_SELECT)
	    is_select = true;
      }

      return rc;
   }

   DLLLOCAL int execute(const char *who, ExceptionSink *xsink);

   DLLLOCAL bool next(ExceptionSink *xsink) {
      return fetch(xsink) ? false : true;
   }

   DLLLOCAL QoreHashNode *fetchRow(OraResultSet &columns, ExceptionSink *xsink);

   DLLLOCAL QoreListNode *fetchRows(OraResultSet &columns, int rows, ExceptionSink *xsink);
   DLLLOCAL QoreListNode *fetchRows(ExceptionSink *xsink);

   DLLLOCAL QoreHashNode *fetchColumns(OraResultSet &columns, int rows, ExceptionSink *xsink);
   DLLLOCAL QoreHashNode *fetchColumns(ExceptionSink *xsink);

   DLLLOCAL Datasource *getDatasource() const {
      return ds;
   }

   DLLLOCAL const QoreEncoding *getEncoding() const {
      return ds->getQoreEncoding();
   }

   DLLLOCAL QoreOracleConnection *getData() const {
      return (QoreOracleConnection *)ds->getPrivateData();
   }
};

#endif
