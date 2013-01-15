#!/usr/bin/env qore

%new-style
%requires oracle

const MSG_COUNT = 5;
const THREAD_COUNT = 5;

const CONNS = (
    "qube" : "omquser2/omquser2@qube",
    "stimpy" : "omquser/omquser@stimpy",
    "rimmer.local" : "omquser/omquser@xe%oraclexe11:1521",
    "vladkyne-karen" : "test/test@orcl%192.168.2.107:1521"
);
our string conn = CONNS{gethostname()};
printf("Hostname: %s; Connection: %s\n", gethostname(), conn);

our hash h;
our Counter c();

class MyAQQueue inherits AQQueue {
    constructor(string q1, string t1, string c1) : AQQueue(q1, t1, c1) {
    }

    log(string fmt) {
	printf("%s T%d async msg: %s\n", format_date("YYYY-MM-DD HH:mm:SS.xx", now_us()), gettid(), vsprintf(fmt, argv));
    }

    nothing onAsyncMessage() {
        on_success commit();
        on_error rollback();
        *AQMessage m1 = getMessage(1);
        log("%y", m1 ? m1.getObject() : NOTHING);
	if (m1) {
	    c.dec();
	    hash msg = m1.getObject();
	    delete h.(msg.CODE);
	}
    }
}

MyAQQueue sub get_queue() {
    return new MyAQQueue("testaq", "test_aq_msg_obj", conn);
}

main();

sub main() {
    # first purge queue
    MyAQQueue q1 = get_queue();
    {
	on_success q1.commit();
	on_error q1.rollback();

	printf("purging queue...\n");
	while ((*AQMessage dm = q1.getMessage())) {
	    printf("purging: msg: %y (%y)\n", dm, dm.getObject());
	}
    }

    q1.startSubscription();

    Counter sc();
    for (int i = 0; i < THREAD_COUNT; ++i) {
	sc.inc();
	background test(i, sc);
    }
    # wait until all msgs have been posted to start waiting until the queue is empty
    sc.waitForZero();

    # wait until the queue is empty
    c.waitForZero(10s);
    printf("not received: %y\n", h.keys());
}

sub test(int i, Counter sc) {
    on_exit sc.dec();

    #AQQueue q("omquser.testaq", "test_aq_msg_obj", conn);
    #AQQueue q("OMQUSER.TESTAQ", "TEST_AQ_MSG_OBJ", conn);
    MyAQQueue q1 = get_queue();
    on_error q1.rollback();

    AQMessage m1();
    #printf("MSG> %N\n", m1);
    hash objin = ("MESSAGE" : "from qorus", "CODE" : i * 1000);

    for (int i = 0; i < MSG_COUNT; ++i) {
	h.(objin.CODE) = True;
	m1.setObject(bindOracleObject("test_aq_msg_obj", objin));
	c.inc();
	q1.post(m1);
	q1.commit();
	++objin.CODE;
    }
}
