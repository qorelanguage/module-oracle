#ifndef AQQUEUEPRIV_H
#define AQQUEUEPRIV_H

#include <qore/Qore.h>
#include "oracle.h"
#include "ocilib_internal.h"


class AQMessagePriv;



DLLEXPORT extern qore_classid_t CID_AQQUEUE;
DLLLOCAL extern QoreClass* QC_AQQUEUE;


#warning "TODO/FIXME: AbstractThreadResource?"
class AQQueuePriv : public AbstractPrivateData {
   protected:
      DLLLOCAL virtual ~AQQueuePriv();

   public:
      DLLLOCAL AQQueuePriv(Datasource *ds, const char *tname, const char *qname, ExceptionSink *xsink);
//      DLLLOCAL AQQueuePriv(QoreOracleConnection *conn, OCI_Msg *msg);

      OCI_TypeInfo* typeInfo() { return m_typeInfo; }
      QoreOracleConnection* connection() { return m_conn; }
      DateTimeNode *getDate(OCIDate* handle) { return m_conn->getDate(handle); }

      bool commit(ExceptionSink *xsink);
      bool rollback(ExceptionSink *xsink);

      bool postMessage(AQMessagePriv *message, ExceptionSink *xsink);
      bool postObject(const QoreHashNode *h, ExceptionSink *xsink);

      QoreObject* getMessage(ExceptionSink *xsink);

      void startSubscription(QoreObject *qo, int64 port, int64 timeout, ExceptionSink *xsink);
      void stopSubscription(ExceptionSink *xsink);

   private:
      QoreOracleConnection *m_conn;
      Datasource *m_ds;

      const char * m_qname;
      OCI_TypeInfo *m_typeInfo;
//      OCI_Msg *m_message;
      OCI_Enqueue *m_enqueue;
      OCI_Dequeue *m_dequeue;

      bool checkDequeue(ExceptionSink *xsink);
};

#endif

