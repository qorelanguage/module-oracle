#!/usr/bin/env qore

%require-our

our hash $thash;
our int $errors;
our hash $o.verbose = True;
#$o.info = True;

sub main() {
    my DatasourcePool $ds("oracle", "omq", "omq", "stimpy11");

    my SQLStatement $drop_stmt($ds);
    $drop_stmt.prepare("drop table test");

    my SQLStatement $insert_stmt($ds);
    $insert_stmt.prepare("insert into test values (%v, %v, %v, %v, %v)");

    my SQLStatement $select_stmt($ds);
    $select_stmt.prepare("select * from test");

    my SQLStatement $select_into_stmt($ds);
    $select_into_stmt.prepare("begin select sysdate into :dt from dual; end;");

    my SQLStatement $select_err_stmt($ds);
    $select_err_stmt.prepare("select * from blah");

    my SQLStatement $delete_stmt($ds);
    $delete_stmt.prepare("delete from test");

    my SQLStatement $plsql($ds);
    $plsql.prepare("
declare
    var number;
begin
    select count(1) into var from user_objects where status = %v;
    :an_output := var;
end;");

    create_table($drop_stmt);
    test1($insert_stmt, $select_stmt);
    test2($select_into_stmt);
    test3($select_err_stmt);
    test4($delete_stmt, $select_stmt);
    test5($plsql);
}

sub info(string $msg) {
    if ($o.info)
	vprintf($msg + "\n", $argv);
}

sub test_value(any $v1, any $v2, string $msg) {
    if ($v1 === $v2) {
	if ($o.verbose)
	    printf("OK: %s test\n", $msg);
    }
    else {
        printf("ERROR: %s test failed! (%N != %N)\n", $msg, $v1, $v2);
        #printf("%s%s", dbg_node_info($v1), dbg_node_info($v2));
        ++$errors;
    }
    $thash.$msg = True;
}

sub create_table(SQLStatement $stmt) {
    try {
	$stmt.exec();
    }
    catch () {
	info("ignoring drop table error");
    }

    $stmt.prepare("create table test (ts timestamp, tstz timestamp with time zone, tsltz timestamp with local time zone, d date, r raw(64))");
    $stmt.exec();
}

sub test1(SQLStatement $insert_stmt, SQLStatement $select_stmt) {
    info("test4");
    #TimeZone::setRegion("America/Chicago");

    my date $now = now_us();
    info("current time: %n", $now);

    my binary $bin = binary("hello");

    {
	info("starting insert");

	my code $insert = sub () {
	    $insert_stmt.bind($now, $now, $now, $now, $bin);
	    info("bind done");

	    $insert_stmt.beginTransaction();
	    on_success $insert_stmt.commit();
	    $insert_stmt.exec();
	    test_value($insert_stmt.affectedRows(), 1, "affected rows after insert");
	};

	$insert();

	$now = now_us();
	$insert();

	$now = now_us();
	$insert();

	$now = now_us();
	$insert();
    }
    info("commit done");

    on_exit $select_stmt.close();

    $select_stmt.exec();
    my int $rows;
    while ($select_stmt.next()) {
	++$rows;
	my hash $rv = $select_stmt.fetchRow();
	info("row=%n", $rv);
    }

    test_value($rows, 4, "next and fetch");

    $select_stmt.close();
    info("closed");

    $select_stmt.exec();
    my any $v = $select_stmt.fetchRows(-1);
    test_value(elements $v, 4, "fetch rows");
    info("rows=%N", $v);

    $select_stmt.close();
    info("closed");

    $select_stmt.exec();
    $v = $select_stmt.fetchColumns(-1);
    test_value(elements $v, 5, "fetch columns");
    info("columns=%N", $v);
}

sub test2(SQLStatement $stmt) {
    info("test2");
    #on_exit $stmt.close();

    $stmt.bindPlaceholders(Type::Date);
    #$stmt.exec();
    my any $v = $stmt.getOutput();
    test_value(elements $v, 1, "first get output");
    info("output: %n", $v);

    $stmt.close();
    info("closed");

    $stmt.bindPlaceholders(Type::Date);
    #$stmt.exec();
    $v = $stmt.getOutput();
    test_value(elements $v, 1, "second get output");
    info("output: %n", $v);
}

sub test3(SQLStatement $stmt) {
    info("test3");
    try {
	$stmt.beginTransaction();
	info("begin transaction");
	$stmt.exec();
	test_value(True, False, "first negative select");
    }
    catch ($ex) {
	info("%s: %s", $ex.err, $ex.desc);
	test_value(True, True, "first negative select");
    }
    try {
	on_success $stmt.commit();
	on_error $stmt.rollback();
	my any $rv = $stmt.fetchColumns(-1);
	info("ERR rows=%N", $rv);
	test_value(True, False, "second negative select");
    }
    catch ($ex) {
	info("%s: %s", $ex.err, $ex.desc);
	test_value(True, True, "second negative select");
    }
    info("rollback done");
}

sub test4(SQLStatement $delete_stmt, SQLStatement $select_stmt) {
    info("test4");
    {
	$delete_stmt.beginTransaction();
	on_success $delete_stmt.commit();
	$delete_stmt.exec();
	test_value(4, $delete_stmt.affectedRows(), "affected rows");
	$delete_stmt.close();
    }
    info("commit done");

    $select_stmt.exec();
    my any $rv = $select_stmt.fetchRows();
    test_value($rv, (), "select after delete");
    #info("test: %N", $stmt.fetchRows());
}

sub test5(SQLStatement $plsql) {
    info("test5");
    {
        $plsql.beginTransaction();
        on_success $plsql.commit();
        my hash $ret = $plsql.execArgs( ('VALID', Type::String) );
        printf("%N\n", $ret);
        $plsql.close();
    }
    info("fetch done");
}

main();
