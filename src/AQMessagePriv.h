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

      DLLLOCAL QoreHashNode* getObject();
      DLLLOCAL void setObject(const QoreHashNode* obj=0);

      DLLLOCAL int64 getAttemptCount();

      DLLLOCAL int64 getEnqueueDelay();
      DLLLOCAL void setEnqueueDelay(int64 delay);

//      OCI_Date* getEnqueueTime();

      DLLLOCAL int64 getExpiration();
      DLLLOCAL void setExpiration(int64 expiration );

      DLLLOCAL int64 getState();

      DLLLOCAL int getPriority();
      DLLLOCAL void setPriority(int64 prio);

//      DLLLOCAL void getID(void *id, unsigned int *len);

      DLLLOCAL const QoreString* getCorrelation();
      DLLLOCAL void setCorrelation(const QoreString* c);


   private:
      QoreHashNode *m_obj;
      int64 m_attemptCount;
      int64 m_enqueueDelay;
      int64 m_expiration;
      int64 m_state;
      int64 m_priority;
      const QoreString *m_correlation;
};

#endif

