/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  AQQueuePriv.h

  Qore Programming Language

  Copyright (C) 2003 - 2012 Qore Technologies, sro

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

#ifndef AQQUEUEPRIV_H
#define AQQUEUEPRIV_H

#include <qore/Qore.h>
#include "oracle.h"
#include "ocilib_internal.h"

class AQMessagePriv;

DLLLOCAL extern qore_classid_t CID_AQQUEUE;
DLLLOCAL extern QoreClass* QC_AQQUEUE;

class AQQueuePriv : public AbstractPrivateData {
protected:
   // mutex for atomicity
   QoreThreadLock l;
   DLLLOCAL virtual ~AQQueuePriv();

public:
   DLLLOCAL AQQueuePriv(Datasource *ds, const QoreStringNode* tname, const QoreStringNode* qname, ExceptionSink *xsink);
   DLLLOCAL virtual void deref(ExceptionSink* xsink);

   DLLLOCAL void connect(ExceptionSink *xsink);
   DLLLOCAL void disconnect(ExceptionSink *xsink);
   DLLLOCAL bool ping();

   DLLLOCAL OCI_TypeInfo* typeInfo() { return m_typeInfo; }
   DLLLOCAL QoreOracleConnection* connection() { return m_conn; }
   DLLLOCAL DateTimeNode *getDate(OCIDate* handle) { return m_conn->getDate(handle); }

   DLLLOCAL bool commit(ExceptionSink *xsink);
   DLLLOCAL bool rollback(ExceptionSink *xsink);

   DLLLOCAL bool postMessage(AQMessagePriv *message, ExceptionSink *xsink);
   DLLLOCAL bool postObject(const QoreHashNode *h, ExceptionSink *xsink);

   DLLLOCAL QoreObject* getMessage(ExceptionSink *xsink);

   DLLLOCAL void startSubscription(QoreObject *qo, int64 port, int64 timeout, ExceptionSink *xsink);

   // returns -1 if an exception was raised, 0 if not: note: no error is raised if there is no active subscription
   DLLLOCAL int stopSubscription(ExceptionSink *xsink);

private:
   QoreOracleConnection *m_conn;
   Datasource *m_ds;

   QoreStringNode* m_tname;
   QoreStringNode* m_qname;
   OCI_TypeInfo *m_typeInfo;
   OCI_Enqueue *m_enqueue;
   OCI_Dequeue *m_dequeue;
   bool m_hasSubscription;

   DLLLOCAL int stopSubscriptionUnlocked(ExceptionSink *xsink);
   DLLLOCAL void disconnectUnlocked(ExceptionSink *xsink);
   DLLLOCAL bool checkDequeueUnlocked(ExceptionSink *xsink);
};

#endif

