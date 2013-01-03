#!/usr/bin/env qore

%new-style
%requires oracle


string conn = (gethostname() == "qube") 
    ? "omquser2/omquser2@qube"
    : "omquser/omquser@stimpy";

class MyAQQueue inherits AQQueue
{
    constructor(string q1, string t1, string c1) : AQQueue(q1, t1, c1) {
    }

    nothing onAsyncMessage() {
        on_success commit();
        on_error rollback();
        *AQMessage m = getMessage();
        printf("msg fetch: %N\n", m);
        printf("     hash: %y\n", m.getObject());
    }
}


#AQQueue q("omquser.testaq", "test_aq_msg_obj", conn);
#AQQueue q("OMQUSER.TESTAQ", "TEST_AQ_MSG_OBJ", conn);
MyAQQueue q("testaq", "test_aq_msg_obj", conn);
on_success q.commit();
on_error q.rollback();

q.startSubscription();

sleep(2);

#printf("QUEUE> %N\n", q);

AQMessage m();
printf("MSG> %N\n", m);
hash objin = ( "MESSAGE" : "from qorus" ,  "CODE" : 666);
m.setObject(bindOracleObject("test_aq_msg_obj", objin));
q.postMessage(m);
q.commit();
++objin.CODE;
m.setObject(bindOracleObject("test_aq_msg_obj", objin));
q.postMessage(m);
q.commit();
++objin.CODE;
m.setObject(bindOracleObject("test_aq_msg_obj", objin));
q.postMessage(m);
q.commit();
++objin.CODE;
m.setObject(bindOracleObject("test_aq_msg_obj", objin));
q.postMessage(m);
q.commit();

printf("get msg...\n");
*AQMessage dm = q.getMessage();
printf("   msg: %N\n", dm);
if (exists dm)
    printf("%N\n", dm.getObject());

++objin.CODE;
m.setObject(bindOracleObject("test_aq_msg_obj", objin));
q.postMessage(m);
q.commit();

++objin.CODE;
m.setObject(bindOracleObject("test_aq_msg_obj", objin));
q.postMessage(m);
q.commit();

sleep(5s);


