%new-style
%requires oracle

/*
hash conn = (
#    "type" : "oracle",
    "user" : "omquser",
    "pass" : "omquser",
    "db"   : "stimpy",
);
*/
hash conn = (
    "user" : "test",
    "pass" : "test",
    "db"   : "xe",
    "host" : "localhost",
    "port" : 1521,
);

#AQQueue q("omquser.testaq", "test_aq_msg_obj", conn);
#AQQueue q("OMQUSER.TESTAQ", "TEST_AQ_MSG_OBJ", conn);
AQQueue q("testaq", "test_aq_msg_obj", conn);
on_success q.commit();
on_error q.rollback();

#printf("QUEUE> %N\n", q);

#AQMessage m();
#printf("MSG> %N\n", m);
#hash objin = ( "MESSAGE" : "from qorus" ,  "CODE" : 666);
#m.setObject(bindOracleObject("test_aq_msg_obj", objin));
#q.postMessage(m);


printf("get msg...\n");
*AQMessage dm = q.getMessage();
printf("   msg: %N\n", dm);
if (exists dm)
    printf("%N\n", dm.getObject());

#q.startSubscription();

#sleep(10s);


