#!/usr/bin/env qore

# database test script
# databases users must be able to create and destroy tables and procedures, etc
# in order to execute all tests

%require-our
%enable-all-warnings
%new-style
%require-types
    
our hash o;
our int errors;
our int test_count;

const opts = (
    "help"    : "h,help",
    "verbose" : "v,verbose:i+",
    "leave"   : "l,leave"
 );

sub usage() {
    printf("usage: %s <connection-string> [options]
 -h,--help            this help text
 -v,--verbose         more v's = more information
 -l,--leave           leave test tables in schema at end\n",
           basename(ENV."_"));
    exit();
}

const object_map = (
    "oracle" : 
    ( "tables" : ora_tables ),
    );

const ora_tables = ( 
    "family" : "create table family (
   family_id int not null,
   name varchar2(80) not null
)",
    "people" : "create table people (
   person_id int not null,
   family_id int not null,
   name varchar2(250) not null,
   dob date not null,
   info clob not null
)",
    "attributes" : "create table attributes (
   person_id int not null,
   attribute varchar2(80) not null,
   value varchar2(160) not null
)" );

sub parse_command_line() {
    GetOpt g(opts);
    o = g.parse(\ARGV);
    if (o.help)
        usage();

    our *string connstr = shift ARGV;

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
    else if (connstr !~ /^\w+:/)
        connstr = "oracle:" + connstr;

    if (ARGV) {
        stderr.printf("excess arguments on command-line (%y): -h for help\n", ARGV);
        exit(1);
    }
}

sub create_datamodel(Datasource db) {
    drop_test_datamodel(db);

    string driver = db.getDriverName();
    # create tables
    hash tables = object_map{driver}.tables;

    foreach string table in (tables.keyIterator()) {
        tprintf(2, "creating table %y\n", table);
        db.exec(tables{table});
    }

    # create procedures if any
    foreach string proc in (object_map{driver}.procs.keyIterator()) {
        tprintf(2, "creating procedure %y\n", proc);
        db.exec(object_map{driver}.procs{proc});
    }

    # create functions if any
    foreach string func in (object_map{driver}.funcs.keyIterator()) {
        tprintf(2, "creating function %y\n", func);
        db.exec(object_map{driver}.funcs{func});
    }

    db.exec("insert into family values ( 1, 'Smith' )");
    db.exec("insert into family values ( 2, 'Jones' )");

    # we insert the dates here using binding by value so we don't have
    # to worry about each database's specific date format
    db.exec("insert into people values ( 1, 1, 'Arnie', %v, 'x')", 1983-05-13);
    db.exec("insert into people values ( 2, 1, 'Sylvia', %v, 'x')", 1994-11-10);
    db.exec("insert into people values ( 3, 1, 'Carol', %v, 'x')", 2003-07-23);
    db.exec("insert into people values ( 4, 1, 'Bernard', %v, 'x')", 1979-02-27);
    db.exec("insert into people values ( 5, 1, 'Isaac', %v, 'x')", 2000-04-04);
    db.exec("insert into people values ( 6, 2, 'Alan', %v, 'x')", 1992-06-04);
    db.exec("insert into people values ( 7, 2, 'John', %v, 'x')", 1995-03-23);

    db.exec("insert into attributes values ( 1, 'hair', 'blond' )");
    db.exec("insert into attributes values ( 1, 'eyes', 'hazel' )");
    db.exec("insert into attributes values ( 2, 'hair', 'blond' )");
    db.exec("insert into attributes values ( 2, 'eyes', 'blue' )");
    db.exec("insert into attributes values ( 3, 'hair', 'brown' )");
    db.exec("insert into attributes values ( 3, 'eyes', 'grey')");
    db.exec("insert into attributes values ( 4, 'hair', 'brown' )");
    db.exec("insert into attributes values ( 4, 'eyes', 'brown' )");
    db.exec("insert into attributes values ( 5, 'hair', 'red' )");
    db.exec("insert into attributes values ( 5, 'eyes', 'green' )");
    db.exec("insert into attributes values ( 6, 'hair', 'black' )");
    db.exec("insert into attributes values ( 6, 'eyes', 'blue' )");
    db.exec("insert into attributes values ( 7, 'hair', 'brown' )");
    db.exec("insert into attributes values ( 7, 'eyes', 'brown' )");
    db.commit();
}

sub drop_test_datamodel(Datasource db) {
    string driver = db.getDriverName();
    # drop the tables and ignore exceptions
    # the commits are needed for databases like postgresql, where errors will prohibit and further
    # actions from being taken on the Datasource
    foreach string table in (object_map{driver}.tables.keyIterator())
        try {
            db.exec("drop table " + table);
            db.commit();
            tprintf(2, "dropped table %y\n", table);
        }
        catch () {
            db.commit();
        }

    # drop procedures and ignore exceptions
    foreach string proc in (object_map{driver}.procs.keyIterator()) {
        *string cmd = object_map{driver}.drop_proc_cmd;
        if (!cmd)
            cmd = "drop procedure";
        try {
            db.exec(cmd + " " + proc);
            db.commit();
            tprintf(2, "dropped procedure %y\n", proc);
        }
        catch () {
            db.commit();
        }
    }

    # drop functions and ignore exceptions
    foreach string func in (object_map{driver}.funcs.keyIterator()) {
        *string cmd = object_map{driver}.drop_func_cmd;
        if (!cmd)
            cmd = "drop function";
        try {
            db.exec(cmd + " " + func);
            db.commit();
            tprintf(2, "dropped function %y\n", func);
        }
        catch () {
            db.commit();
        }
    }
}

Datasource sub getDS() {
    return new Datasource(connstr);
}

sub tprintf(int v, string msg) {
    if (v <= o.verbose)
        vprintf(msg, argv);
}

sub test_value(any v1, any v2, string msg) {
    ++test_count;
    if (v1 == v2)
        tprintf(1, "OK: %s test\n", msg);
    else {
        tprintf(0, "ERROR: %s test failed! (%y != %y)\n", msg, v1, v2);
        errors++;
    }
}

const family_hash = (
    "Jones" : (
        "people" : (
            "John" : (
                "info": "x",
                "dob" : 1995-03-23,
                "eyes" : "brown",
                "hair" : "brown" ),
            "Alan" : (
                "info": "x",
                "dob" : 1992-06-04,
                "eyes" : "blue",
                "hair" : "black" ) ) ),
    "Smith" : (
        "people" : (
            "Arnie" : (
                "info": "x",
                "dob" : 1983-05-13,
                "eyes" : "hazel",
                "hair" : "blond" ),
            "Carol" : (
                "info": "x",
                "dob" : 2003-07-23,
                "eyes" : "grey",
                "hair" : "brown" ),
            "Isaac" : (
                "info": "x",
                "dob" : 2000-04-04,
                "eyes" : "green",
                "hair" : "red" ),
            "Bernard" : (
                "info": "x",
                "dob" : 1979-02-27,
                "eyes" : "brown",
                "hair" : "brown" ),
            "Sylvia" : (
                "info": "x",
                "dob" : 1994-11-10,
                "eyes" : "blue",
                "hair" : "blond" ) ) ) );

sub context_test(Datasource db) {
    # first we select all the data from the tables and then use 
    # context statements to order the output hierarchically

    # context statements are most useful when a set of queries can be executed once
    # and the results processed many times by creating "views" with context statements

    *hash people = db.select("select * from people");
    *hash attributes = db.select("select * from attributes");

    # in this test, we create a big hash structure out of the queries executed above
    # and compare it at the end to the expected result

    # display each family sorted by family name
    hash fl;
    context family (db.select("select * from family")) sortBy (%name) {
        hash pl;
        tprintf(2, "Family %d: %s\n", %family_id, %name);

        # display people, sorted by eye color, descending
        context people (people)
            sortDescendingBy (find %value in attributes
                              where (%attribute == "eyes"
                                     && %person_id == %people:person_id))
            where (%family_id == %family:family_id) {
            hash al;
            tprintf(2, "  %s, born %s\n", %name, format_date("Month DD, YYYY", %dob));
            context (attributes) sortBy (%attribute) where (%person_id == %people:person_id) {
                al.%attribute = %value;
                tprintf(2, "    has %s %s\n", %value, %attribute);
            }
            # leave out the ID fields and name from hash under name; subtracting a
            # string from a hash removes that key from the result
            # this is "doing it the hard way", there is only one key left,
            # "dob", then attributes are added directly into the person hash
            pl.%name = %% - "family_id" - "person_id" - "name" + al;
        }
        # leave out family_id and name fields (leaving an empty hash)
        fl.%name = %% - "family_id" - "name" + ( "people" : pl );
    }

    # test context ordering
    test_value(fl.keys(), ("Jones", "Smith"), "first context");
    test_value(fl.Smith.people.keys(), ("Arnie", "Carol", "Isaac", "Bernard", "Sylvia"), "second context");
    # test entire context value
    test_value(fl, family_hash, "third context");
}

sub test_timeout(Datasource db, Counter c) {
    db.setTransactionLockTimeout(1ms);
    try {
        # this should cause a TRANSACTION-LOCK-TIMEOUT exception to be thrown
        db.exec("insert into family values (3, 'Test')\n");
        test_value(True, False, "transaction timeout");
        db.exec("delete from family where name = 'Test'");
    }
    catch (ex) {
        test_value(True, True, "transaction timeout");
    }
    # signal parent thread to continue
    c.dec();
}

sub transaction_test(Datasource db) {
    Datasource ndb = getDS();
    any r;
    tprintf(2, "db.autocommit=%N, ndb.autocommit=%N\n", db.getAutoCommit(), ndb.getAutoCommit());

    # first, we insert a new row into "family" but do not commit it
    int rows = db.exec("insert into family values (3, 'Test')\n");
    if (rows !== 1)
        printf("FAILED INSERT, rows=%N\n", rows);

    # now we verify that the new row is not visible to the other datasource
    # unless it's a sybase/ms sql server datasource, in which case this would deadlock :-(
    if (o.type != "sybase" && o.type != "mssql") {
        r = ndb.selectRow("select name from family where family_id = 3").name;
        test_value(r, NOTHING, "first transaction");
    }

    # now we verify that the new row is visible to the inserting datasource
    r = db.selectRow("select name from family where family_id = 3").name;
    test_value(r, "Test", "second transaction");

    # test datasource timeout
    # this Counter variable will allow the parent thread to sleep
    # until the child thread times out
    Counter c(1);
    background test_timeout(db, c);

    # wait for child thread to time out
    c.waitForZero();

    # now, we commit the transaction
    db.commit();

    # now we verify that the new row is visible in the other datasource
    r = ndb.selectRow("select name from family where family_id = 3").name;
    test_value(r, "Test", "third transaction");

    # now we delete the row we inserted (so we can repeat the test)
    r = ndb.exec("delete from family where family_id = 3");
    test_value(r, 1, "delete row count");
    ndb.commit();
}

const family_q = ( "family_id" : 1,
                   "name" : "Smith" );
const person_q = ( "person_id" : 1,
                   "family_id" : 1,
                   "name" : "Arnie",
                   "dob" : 1983-05-13 );
const params = ( "string" : "hello there",
                 "int" : 150 );

sub main() {
    parse_command_line();
    Datasource db = getDS();

    string driver = db.getDriverName();
    printf("testing %s driver\n", driver);
    string sv = db.getServerVersion();
    if (o.verbose > 1)
        tprintf(2, "client version=%y\nserver version=%y\n", db.getClientVersion(), sv);

    create_datamodel(db);

    context_test(db);
    transaction_test(db);

    if (!o.leave)
        drop_test_datamodel(db);
    printf("%d/%d tests OK\n", test_count - errors, test_count);
}

main();
