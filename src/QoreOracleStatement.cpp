/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  QoreOracleStatement.cpp

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

#include "oracle.h"

int QoreOracleStatement::execute(const char *who, unsigned iters, ExceptionSink *xsink) {
   OracleData *d_ora = (OracleData *)ds->getPrivateData();

   int status = OCIStmtExecute(d_ora->svchp, stmthp, d_ora->errhp, iters, 0, 0, 0, OCI_DEFAULT);

   //printd(0, "QoreOracleStatement::execute() stmthp=%p status=%d (OCI_ERROR=%d)\n", stmthp, status, OCI_ERROR);
   if (status == OCI_ERROR) {
      // see if server is connected
      ub4 server_status = 0;

      // get server handle
      OCIServer *svrh;
      if (d_ora->checkerr(OCIAttrGet(d_ora->svchp, OCI_HTYPE_SVCCTX, &svrh, 0, OCI_ATTR_SERVER, d_ora->errhp), who, xsink))
	 return -1;

      //printd(0, "QoreOracleStatement::execute() got server handle: %p\n", svrh);
      if (d_ora->checkerr(OCIAttrGet(svrh, OCI_HTYPE_SERVER, (dvoid *)&server_status, 0, OCI_ATTR_SERVER_STATUS, d_ora->errhp), who, xsink))
	 return -1;

      //printd(0, "QoreOracleStatement::execute() server_status=%d (OCI_SERVER_NOT_CONNECTED=%d)\n", server_status, OCI_SERVER_NOT_CONNECTED);
      if (server_status == OCI_SERVER_NOT_CONNECTED) {
	 // check if a transaction was in progress
	 if (ds->isInTransaction()) {
	    ds->connectionAborted();
	    xsink->raiseException("DBI:ORACLE:TRANSACTION-ERROR", "connection to Oracle database server lost while in a transaction; transaction has been lost");
	    return -1;
	 }

	 // otherwise try to reconnect
	 OCILogoff(d_ora->svchp, d_ora->errhp);

	 //printd(0, "QoreOracleStatement::execute() about to execute OCILogon() for reconnect\n");
	 if (d_ora->checkerr(OCILogon(d_ora->envhp, d_ora->errhp, &d_ora->svchp, (text *)ds->getUsername(), strlen(ds->getUsername()), (text *)ds->getPassword(), strlen(ds->getPassword()), (text *)ds->getDBName(), strlen(ds->getDBName())), who, xsink)) {
	    // close datasource and remove private data
	    ds->close();
	    return -1;
	 }

	 //printd(0, "QoreOracleStatement::execute() returned from OCILogon() status=%d\n", status);
	 status = OCIStmtExecute(d_ora->svchp, stmthp, d_ora->errhp, iters, 0, 0, 0, OCI_DEFAULT);
	 if (status && d_ora->checkerr(status, who, xsink))
	    return -1;
      }
      else {
	 //printd(0, "QoreOracleStatement::execute() error, but it's connected; status=%d who=%s\n", status, who);
	 d_ora->checkerr(status, who, xsink);
	 return -1;
      }
   }
   else if (status && d_ora->checkerr(status, who, xsink))
      return -1;

   return 0;      
}

QoreListNode *QoreOracleStatement::fetchRows(ExceptionSink *xsink) {
   // setup column structure for output columns
   ColumnHelper columns(*this, "QoreOracleStatement::fetchRows():params", xsink);
   if (*xsink)
      return 0;

   ReferenceHolder<QoreListNode> l(new QoreListNode, xsink);

   // setup temporary row to accept values
   columns->define("QoreOracleStatement::fetchRows():define", xsink);

   // now finally fetch the data
   while (true) {
      if (fetch(xsink))
	 break;

      // set up hash for row
      ReferenceHolder<QoreHashNode> h(new QoreHashNode, xsink);

      // copy data or perform per-value processing if needed
      for (clist_t::iterator i = columns->clist.begin(), e = columns->clist.end(); i != e; ++i) {
	 OraColumn *w = *i;
	 // assign value to hash
	 h->setKeyValue(w->name, w->getValue(true, xsink), xsink);
	 if (*xsink)
	    break;
      }
      // add row to list
      l->push(h.release());
   }
   //printd(2, "QoreOracleStatement::fetchRows(): %d column(s), %d row(s) retrieved as output\n", columns->size(), l->size());
   return *xsink ? 0 : l.release();
}

QoreHashNode *QoreOracleStatement::fetchColumns(ExceptionSink *xsink) {
   // retrieve results from statement and return hash
   ColumnHelper columns(*this, "QoreOracleStatement::fetchColumns():params", xsink);
   if (*xsink)
      return 0;
   
   // allocate result hash for result value
   ReferenceHolder<QoreHashNode> h(new QoreHashNode, xsink);
      
   // create hash elements for each column, assign empty list
   for (clist_t::iterator i = columns->clist.begin(), e = columns->clist.end(); i != e; ++i) {
      //printd(5, "QoreOracleStatement::fetchColumns() allocating list for '%s' column\n", w->name);
      h->setKeyValue((*i)->name, new QoreListNode, xsink);
   }
   
   //int num_rows = 0;
   
   // setup temporary row to accept values
   columns->define("QoreOracleStatement::fetchColumns():define", xsink);
   
   // now finally fetch the data
   while (true) {
      if (fetch(xsink))
	 break;
      
      // copy data or perform per-value processing if needed
      for (unsigned i = 0; i < columns->clist.size(); ++i) {
	 OraColumn *w = columns->clist[i];
	 // get pointer to value of target node
	 QoreListNode *l = reinterpret_cast<QoreListNode *>(h->getKeyValue(w->name, xsink));
	 if (!l)
	    break;
	 l->push(w->getValue(false, xsink));
	 if (*xsink)
	    break;
      }

      //++num_rows;
   }
   //printd(2, "QoreOracleStatement::fetchColumns(): %d column(s), %d row(s) retrieved as output\n", columns->size(), num_rows);
   return *xsink ? 0 : h.release();
}
