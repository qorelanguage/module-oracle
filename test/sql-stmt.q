#!/usr/bin/env qore

%require-our
%new-style

our hash thash;
our int errors;

const opts = (
    "help": "h,help",
    "info": "i,info",
    "verbose" : "v,verbose:i+",
 );

sub usage() {
   printf("usage: %s <connection-string> [options]
 -h,--help            this help text
 -i,--info            show informational messages
 -v,--verbose         more v's = more information
",
           get_script_name());
    exit();
}

sub process_command_line() {
    GetOpt g(opts);
    our hash o = g.parse(\ARGV);
    if (o.help)
        usage();
}

sub main() {
    process_command_line();

    *string connstr = shift ARGV;

    if (!connstr) {
        connstr = ENV.QORE_DB_CONNSTR;
        if (!connstr) {
            string hn = gethostname();
            switch (hn) {
                case /^star/: connstr = "oracle:omquser/omquser@star"; break;
                case /^quasar/: connstr = "oracle:omquser/omquser@el7"; break;
                case /^el6/:
                case /^el5/:
                case /^manatee/:
                case /^xbox/: connstr = "oracle:omquser/omquser@xbox"; break;
                default: {
                    stderr.printf("cannot detect DB connect string from hostname and QORE_DB_CONNSTR environment variable not set; cannot run SqlUtil tests\n");
                    return;
                }
            }
        }
    }

    if (connstr !~ /^[a-z0-9_]+:/)
        connstr = "oracle:" + connstr;
    DatasourcePool ds(connstr);

    SQLStatement insert_stmt(ds);
    insert_stmt.prepare("insert into test values (%v, %v, %v, %v, %v)");

    SQLStatement select_stmt(ds);
    select_stmt.prepare("select * from test");

    SQLStatement select_into_stmt(ds);
    select_into_stmt.prepare("begin select sysdate into :dt from dual; end;");

    SQLStatement select_err_stmt(ds);
    select_err_stmt.prepare("select * from blah");

    SQLStatement delete_stmt(ds);
    delete_stmt.prepare("delete from test");

    SQLStatement plsql(ds);
    plsql.prepare("
declare
    var number;
begin
    select count(1) into var from user_objects where status = %v;
    :an_output := var;
end;");

    create_table(ds);
    test1(insert_stmt, select_stmt);
    test2(select_into_stmt);
    test3(select_err_stmt);
    test4(delete_stmt, select_stmt);
    test5(plsql);
}

sub info(string msg) {
    if (o.info)
        vprintf(msg + "\n", argv);
}

sub test_value(any v1, any v2, string msg) {
    if (v1 === v2) {
        if (o.verbose)
            printf("OK: %s test\n", msg);
    }
    else {
        printf("ERROR: %s test failed! (%N != %N)\n", msg, v1, v2);
        #printf("%s%s", dbg_node_info(v1), dbg_node_info(v2));
        ++errors;
    }
    thash{msg} = True;
}

sub create_table(DatasourcePool dsp) {
    SQLStatement drop_stmt(dsp);
    drop_stmt.prepare("drop table test");

    on_exit drop_stmt.commit();

    try {
        drop_stmt.exec();
    }
    catch () {
        info("ignoring drop table error");
    }

    drop_stmt.prepare("create table test (ts timestamp, tstz timestamp with time zone, tsltz timestamp with local time zone, d date, r raw(64))");
    drop_stmt.exec();
}

sub test1(SQLStatement insert_stmt, SQLStatement select_stmt) {
    info("test1");
    #TimeZone::setRegion("America/Chicago");

    date now = now_us();
    info("current time: %n", now);

    binary bin = binary("hello");

    {
        info("starting insert");

        code insert = sub () {
            insert_stmt.bind(now, now, now, now, bin);
            info("bind done: %y", now);

            insert_stmt.beginTransaction();
            on_success insert_stmt.commit();
            insert_stmt.exec();
            test_value(insert_stmt.affectedRows(), 1, "affected rows after insert");
        };

        insert();

        now = now_us();
        insert();

        now = now_us();
        insert();

        now = now_us();
        insert();
    }
    info("commit done");

    on_exit select_stmt.commit();

    select_stmt.exec();
    int rows;
    while (select_stmt.next()) {
        ++rows;
        hash rv = select_stmt.fetchRow();
        info("row=%n", rv);
    }

    test_value(rows, 4, "next and fetch");

    select_stmt.close();
    info("closed");

    select_stmt.exec();
    any v = select_stmt.fetchRows(-1);
    test_value(elements v, 4, "fetch rows");
    info("rows=%N", v);

    select_stmt.close();
    info("closed");

    select_stmt.exec();
    v = select_stmt.fetchColumns(-1);
    test_value(elements v, 5, "fetch columns");

    select_stmt.exec();
    v = select_stmt.fetchColumns(-1);
    test_value(elements v, 5, "fetch columns 2");

    info("columns: %N", v);

    select_stmt.exec();

    *list l = select_stmt.fetchRows(3);
    test_value(l.size(), 3, "fetch rows 1");
    l = select_stmt.fetchRows(3);
    test_value(l.size(), 1, "fetch rows 2");
    l = select_stmt.fetchRows(3);
    test_value(l.size(), 0, "fetch rows 3");
    try {
        l = select_stmt.fetchRows(3);
        test_value(True, False, "fetch rows 4");
    }
    catch (hash ex) {
        test_value(ex.err, "ORACLE-SELECT-ROWS-ERROR", "fetch rows 4");
    }
}

sub test2(SQLStatement stmt) {
    info("test2");
    on_exit stmt.commit();

    stmt.bindPlaceholders(Type::Date);
    #stmt.exec();
    any v = stmt.getOutput();
    test_value(elements v, 1, "first get output");
    info("output: %n", v);

    stmt.close();
    info("closed");

    stmt.bindPlaceholders(Type::Date);
    #stmt.exec();
    v = stmt.getOutput();
    test_value(elements v, 1, "second get output");
    info("output: %n", v);
}

sub test3(SQLStatement stmt) {
    info("test3");
    try {
        stmt.beginTransaction();
        info("begin transaction");
        stmt.exec();
        test_value(True, False, "first negative select");
    }
    catch (ex) {
        info("%s: %s", ex.err, ex.desc);
        test_value(True, True, "first negative select");
    }
    try {
        on_success stmt.commit();
        on_error stmt.rollback();
        any rv = stmt.fetchColumns(-1);
        info("ERR rows=%N", rv);
        test_value(True, False, "second negative select");
    }
    catch (ex) {
        info("%s: %s", ex.err, ex.desc);
        test_value(True, True, "second negative select");
    }
    info("rollback done");
}

sub test4(SQLStatement delete_stmt, SQLStatement select_stmt) {
    info("test4");
    {
        delete_stmt.beginTransaction();
        on_success delete_stmt.commit();
        delete_stmt.exec();
        test_value(4, delete_stmt.affectedRows(), "affected rows");
        delete_stmt.close();
    }
    info("commit done");

    select_stmt.exec();
    on_exit select_stmt.commit();
    any rv = select_stmt.fetchRows();
    test_value(rv, (), "select after delete");
    #info("test: %N", stmt.fetchRows());
}

sub test5(SQLStatement plsql) {
    info("test5");
    {
        plsql.beginTransaction();
        on_success plsql.commit();
        plsql.execArgs( ('VALID', Type::String) );
        hash ret = plsql.getOutput();
        #printf("%N\n", ret);
        plsql.close();
    }
    info("fetch done");
}

main();
