#ifndef AQMESSAGEPRIV_H
#define AQMESSAGEPRIV_H

#include <qore/Qore.h>
#include "oracle.h"
#include "ocilib_internal.h"


class AQQueuePriv;


DLLEXPORT extern qore_classid_t CID_AQMESSAGE;
DLLLOCAL extern QoreClass* QC_AQMESSAGE;


class AQMessagePriv : public AbstractPrivateData {
   protected:
      DLLLOCAL virtual ~AQMessagePriv();

   public:
      DLLLOCAL AQMessagePriv(const QoreHashNode* obj = 0);

      QoreHashNode* getObject();
      void setObject(const QoreHashNode* obj=0);

      int64 getAttemptCount();

      int64 getEnqueueDelay();
      void setEnqueueDelay(int64 delay);

//      OCI_Date* getEnqueueTime();

      int64 getExpiration();
      void setExpiration(int64 expiration );

      int64 getState();

      int getPriority();
      void setPriority(int64 prio);

//      void getID(void *id, unsigned int *len);

      const char * getCorrelation();
      void setCorrelation(const char* c);


   private:
      QoreHashNode *m_obj;
      int64 m_attemptCount;
      int64 m_enqueueDelay;
      int64 m_expiration;
      int64 m_state;
      int64 m_priority;
      const char * m_correlation;
};

#endif

