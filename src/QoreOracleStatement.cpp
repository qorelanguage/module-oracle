/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  QoreOracleStatement.cpp

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

static inline bool wasInTransaction(Datasource *ds) {
#ifdef _QORE_HAS_DATASOURCE_ACTIVETRANSACTION
   return ds->activeTransaction();
#else
   return ds->isInTransaction();
#endif
}

int QoreOracleStatement::execute(const char *who, ExceptionSink *xsink) {
   QoreOracleConnection *conn = (QoreOracleConnection *)ds->getPrivateData();

   assert(conn->svchp);
   int status = OCIStmtExecute(conn->svchp, stmthp, conn->errhp, is_select ? 0 : 1, 0, 0, 0, OCI_DEFAULT);

   //printd(0, "QoreOracleStatement::execute() stmthp=%p status=%d (OCI_ERROR=%d)\n", stmthp, status, OCI_ERROR);
   if (status == OCI_ERROR) {
      // see if server is connected
      int ping = OCI_Ping(&conn->ocilib, conn->ocilib_cn, xsink);

      if (*xsink) {
         if (ping)
            ping = 0;
#ifdef DEBUG
         xsink->handleExceptions();
#endif
         xsink->clear();
      }

      if (!ping) {

	 // check if a transaction was in progress
         if (wasInTransaction(ds))
	    xsink->raiseException("DBI:ORACLE:TRANSACTION-ERROR", "connection to Oracle database server lost while in a transaction; transaction has been lost");

	 // try to reconnect
	 conn->logoff();

	 //printd(0, "QoreOracleStatement::execute() about to execute OCILogon() for reconnect\n");
	 if (conn->logon(xsink)) {
            //printd(5, "QoreOracleStatement::execute() conn=%p reconnect failed, marking connection as closed\n", conn);
	    // close datasource and remove private data
	    ds->connectionAborted();
	    return -1;
	 }
         if (conn->checkWarnings(xsink)) {
             //printd(5, "QoreOracleStatement::execute() conn=%p reconnect failed, marking connection as closed\n", conn);
            // close datasource and remove private data
            ds->connectionAborted();
            return -1;
         }


         // don't execute again if the connection was aborted while in a transaction
         if (wasInTransaction(ds))
	    return -1;

	 //printd(0, "QoreOracleStatement::execute() returned from OCILogon() status=%d\n", status);
	 status = OCIStmtExecute(conn->svchp, stmthp, conn->errhp, is_select ? 0 : 1, 0, 0, 0, OCI_DEFAULT);
	 if (status && conn->checkerr(status, who, xsink))
	    return -1;
      }
      else {
	 //printd(0, "QoreOracleStatement::execute() error, but it's connected; status=%d who=%s\n", status, who);
	 conn->checkerr(status, who, xsink);
	 return -1;
      }
   }
   else if (status && conn->checkerr(status, who, xsink))
      return -1;

   return 0;      
}

QoreHashNode *QoreOracleStatement::fetchRow(OraResultSet &resultset, ExceptionSink *xsink) {
   if (!fetch_done) {
      xsink->raiseException("ORACLE-FETCH-ROW-ERROR", "call SQLStatement::next() before calling SQLStatement::fetchRow()");
      return 0;
   }

   // set up hash for row
   ReferenceHolder<QoreHashNode> h(new QoreHashNode, xsink);

   // copy data or perform per-value processing if needed
   for (clist_t::iterator i = resultset.clist.begin(), e = resultset.clist.end(); i != e; ++i) {
      OraColumnBuffer *w = *i;
      // assign value to hash
      AbstractQoreNode* n = w->getValue(true, xsink);
      if (*xsink) {
         assert(!n);
         return 0;
      }
      h->setKeyValue(w->name, n, xsink);
      if (*xsink)
	 return 0;
   }

   return h.release();
}

QoreListNode *QoreOracleStatement::fetchRows(ExceptionSink *xsink) {
   OraResultSetHelper resultset(*this, "QoreOracleStatement::fetchRows():params", xsink);
   if (*xsink)
      return 0;

   return fetchRows(**resultset, -1, xsink);
}

QoreListNode *QoreOracleStatement::fetchRows(OraResultSet &resultset, int rows, ExceptionSink *xsink) {
   ReferenceHolder<QoreListNode> l(new QoreListNode, xsink);

   // setup temporary row to accept values
   if (resultset.define("QoreOracleStatement::fetchRows():define", xsink))
      return 0;

   int num_rows = 0;
   
   // now finally fetch the data
   while (next(xsink)) {
      QoreHashNode *h = fetchRow(resultset, xsink);
      if (!h)
	 return 0;

      // add row to list
      l->push(h);

      ++num_rows;
      if (rows > 0 && num_rows == rows)
	 break;
   }
   //printd(2, "QoreOracleStatement::fetchRows(): %d column(s), %d row(s) retrieved as output\n", resultset.size(), l->size());
   if (!*xsink) {
      if (!fetch_done)
         fetch_done = true;
      return l.release();
   }
   return 0;
}

#ifdef _QORE_HAS_DBI_SELECT_ROW
QoreHashNode *QoreOracleStatement::fetchSingleRow(ExceptionSink *xsink) {
   OraResultSetHelper resultset(*this, "QoreOracleStatement::fetchRow():params", xsink);
   if (*xsink)
      return 0;

   ReferenceHolder<QoreHashNode> rv(xsink);

   // setup temporary row to accept values
   if (resultset->define("QoreOracleStatement::fetchRows():define", xsink))
      return 0;

   //printd(2, "QoreOracleStatement::fetchRow(): %d column(s) retrieved as output\n", resultset->size());

   // now finally fetch the data
   if (!next(xsink))
      return 0;

   rv = fetchRow(**resultset, xsink);
   if (!rv)
      return 0;

   if (!fetch_done)
      fetch_done = true;

   if (next(xsink)) {
      xsink->raiseExceptionArg("ORACLE-SELECT-ROW-ERROR", rv.release(), "SQL passed to selectRow() returned more than 1 row");
      return 0;
   }

   return rv.release();
}
#endif

// retrieve results from statement and return hash
QoreHashNode *QoreOracleStatement::fetchColumns(ExceptionSink *xsink) {
   OraResultSetHelper resultset(*this, "QoreOracleStatement::fetchColumns():params", xsink);
   if (*xsink)
      return 0;

   return fetchColumns(**resultset, -1, xsink);
}

// retrieve results from statement and return hash
QoreHashNode *QoreOracleStatement::fetchColumns(OraResultSet &resultset, int rows, ExceptionSink *xsink) {
   // allocate result hash for result value
   ReferenceHolder<QoreHashNode> h(new QoreHashNode, xsink);
      
   // create hash elements for each column, assign empty list
   for (clist_t::iterator i = resultset.clist.begin(), e = resultset.clist.end(); i != e; ++i) {
      //printd(5, "QoreOracleStatement::fetchColumns() allocating list for '%s' column\n", w->name);
      h->setKeyValue((*i)->name, new QoreListNode, xsink);
   }
   
   // setup temporary row to accept values
   if (resultset.define("QoreOracleStatement::fetchColumns():define", xsink))
      return 0;

   int num_rows = 0;
   
   // now finally fetch the data
   while (next(xsink)) {
      // copy data or perform per-value processing if needed
      for (unsigned i = 0; i < resultset.clist.size(); ++i) {
	 OraColumnBuffer *w = resultset.clist[i];
	 // get pointer to value of target node
	 QoreListNode *l = reinterpret_cast<QoreListNode *>(h->getKeyValue(w->name, xsink));
	 if (!l)
	    break;
         AbstractQoreNode* n = w->getValue(false, xsink);
         if (*xsink) {
            assert(!n);
            break;
         }

	 l->push(n);
	 if (*xsink)
	    break;
      }

      ++num_rows;
      if (rows > 0 && num_rows == rows)
	 break;
   }
   //printd(2, "QoreOracleStatement::fetchColumns(rows: %d): %d column(s), %d row(s) retrieved as output\n", rows, resultset.size(), num_rows);
   if (!*xsink) {
      if (!fetch_done)
         fetch_done = true;
      return h.release();
   }
   return 0;
}
