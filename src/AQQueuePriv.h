#ifndef AQQUEUEPRIV_H
#define AQQUEUEPRIV_H

#include <qore/Qore.h>
#include "oracle.h"
#include "ocilib_internal.h"


class AQMessagePriv;



DLLEXPORT extern qore_classid_t CID_AQQUEUE;
DLLLOCAL extern QoreClass* QC_AQQUEUE;


class AQQueuePriv : public AbstractPrivateData {
   protected:
      DLLLOCAL virtual ~AQQueuePriv();

   public:
      DLLLOCAL AQQueuePriv(Datasource *ds, const char *tname, const char *qname, ExceptionSink *xsink);
//      DLLLOCAL AQQueuePriv(QoreOracleConnection *conn, OCI_Msg *msg);

      DLLLOCAL OCI_TypeInfo* typeInfo() { return m_typeInfo; }
      DLLLOCAL QoreOracleConnection* connection() { return m_conn; }
      DLLLOCAL DateTimeNode *getDate(OCIDate* handle) { return m_conn->getDate(handle); }

      DLLLOCAL bool commit(ExceptionSink *xsink);
      DLLLOCAL bool rollback(ExceptionSink *xsink);

      DLLLOCAL bool postMessage(AQMessagePriv *message, ExceptionSink *xsink);
      DLLLOCAL bool postObject(const QoreHashNode *h, ExceptionSink *xsink);

      DLLLOCAL QoreObject* getMessage(ExceptionSink *xsink);

      DLLLOCAL void startSubscription(QoreObject *qo, int64 port, int64 timeout, ExceptionSink *xsink);
      DLLLOCAL void stopSubscription(ExceptionSink *xsink);

   private:
      QoreOracleConnection *m_conn;
      Datasource *m_ds;

      const char * m_qname;
      OCI_TypeInfo *m_typeInfo;
//      OCI_Msg *m_message;
      OCI_Enqueue *m_enqueue;
      OCI_Dequeue *m_dequeue;

      DLLLOCAL bool checkDequeue(ExceptionSink *xsink);
};

#endif

