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
        *AQMessage m1 = getMessage();
        printf("TID %d: MyAQQueue::onAsyncMessage() msg fetch: %N\n", gettid(), m1);
        printf("TID %d: MyAQQueue::onAsyncMessage()      hash: %y\n", gettid(), m1.getObject());
    }
}

main();

sub main() {
    #AQQueue q("omquser.testaq", "test_aq_msg_obj", conn);
    #AQQueue q("OMQUSER.TESTAQ", "TEST_AQ_MSG_OBJ", conn);
    MyAQQueue q1("testaq", "test_aq_msg_obj", conn);
    on_success q1.commit();
    on_error q1.rollback();

    q1.startSubscription();

    printf("purging queue...\n");
    while ((*AQMessage dm = q1.getMessage())) {
	printf("purging: msg: %y (%N)\n", dm, dm.getObject());
    }

    #printf("QUEUE> %N\n", q1);

    AQMessage m1();
    printf("MSG> %N\n", m1);
    hash objin = ( "MESSAGE" : "from qorus" ,  "CODE" : 666);
    m1.setObject(bindOracleObject("test_aq_msg_obj", objin));
    q1.postMessage(m1);
    q1.commit();
    ++objin.CODE;
    m1.setObject(bindOracleObject("test_aq_msg_obj", objin));
    q1.postMessage(m1);
    q1.commit();
    ++objin.CODE;
    m1.setObject(bindOracleObject("test_aq_msg_obj", objin));
    q1.postMessage(m1);
    q1.commit();
    ++objin.CODE;
    m1.setObject(bindOracleObject("test_aq_msg_obj", objin));
    q1.postMessage(m1);
    q1.commit();
    
    ++objin.CODE;
    m1.setObject(bindOracleObject("test_aq_msg_obj", objin));
    q1.postMessage(m1);
    q1.commit();
    
    ++objin.CODE;
    m1.setObject(bindOracleObject("test_aq_msg_obj", objin));
    q1.postMessage(m1);
    q1.commit();
    
    sleep(5s);
}
