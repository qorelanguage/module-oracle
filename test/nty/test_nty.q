#/usr/bin/env qore
%include test_nty_commons.q
%requires oracle

start();

sub start() {

    my string $connstr = "oracle:omqtest/omqtest@xbox";
    our Datasource $db($connstr);
    $db.open();

    my int $id = 0; 
    my int $prev_id = 0;
    my int $prev_success = 0;

    test_varchar2_in($id++);
    test_varchar2_out($id++);
    test_number_in($id++);
    test_number_out($id++);
    test_clob_in($id++);
    test_clob_out($id++);
	test_date_in($id++);
    test_date_out($id++);
    test_object_collection_in($id++);
    test_object_collection_out($id++);
    test_string_in($id++);
    test_string_out($id++);
    test_objects_in($id++);
    test_objects_out($id++);
    test_timestamp_in($id++);
    test_timestamp_out($id++);
    my list $row_data = "Qore named types test - collections", sprintf("(%d/%d)", $success_count, $id);
    print_row($row_data);
    $prev_id = $id;
    $prev_success = $success_count;
    test_exception1($id++);
    test_exception2($id++);
    test_exception3($id++);
    test_exception4($id++);
    $row_data = "Qore named types test - various errors checking", sprintf("(%d/%d)", $success_count-$prev_success, $id-$prev_id);
    print_row($row_data);
    $prev_id = $id;
    $prev_success = $success_count;

    test_object_insert($id++);
    $row_data = "Qore named types test - objects", sprintf("(%d/%d)", $success_count-$prev_success, $id-$prev_id);
    print_row($row_data);

    $row_data = "TESTING COMPLETED", sprintf("(%d/%d)", $success_count, $id);

    print_row($row_data);
    print_char("-", COL_WIDTH+15);
    printf("\n");

}

sub test_varchar2_in(int id) {
    my hash $c = $db.exec("begin qore_test.do_coll(%v, :retval); end;", bindOracleCollection("COL_TEST", VARCHAR2_LST_IN));
    #print_rslt($id, VARCHAR2_LST_IN, $c);
    print_result($id, "collection IN varchar2", cmp_result(NOTHING, $c.retval, COLL_VARCHAR2_IN), VARCHAR2_LST_IN, $c.retval);
    $db.rollback();
}

sub test_varchar2_out(int id) {
    my $r2 = $db.exec("begin qore_test.get_coll(:retval); end;", placeholderOracleCollection("COL_TEST"));
    #print_rslt($id, VARCHAR2_LST_OUT, $r2);
    print_result($id, "collection OUT varchar2", cmp_result(VARCHAR2_LST_OUT, $r2.retval, COLL_VARCHAR2_OUT), VARCHAR2_LST_OUT, $r2);
    $db.rollback();
}

sub test_number_in(int id) {
    my $c = $db.exec("begin qore_test.do_coll_num(%v, :retval); end;", bindOracleCollection("COL_TEST_NUM", NUM_LST_IN));
    #print_rslt($id, NUM_LST_IN, $c);
    print_result($id, "collection IN number", cmp_result(NOTHING, $c.retval, COLL_NUMBER_IN), NUM_LST_IN, $c.retval);
    $db.rollback();
}

sub test_number_out(int id) {
    my $r2 = $db.exec("begin qore_test.get_coll_num(:retval); end;", placeholderOracleCollection("COL_TEST_NUM"));
    #print_rslt($id, NUM_LST_OUT, $r2);
    print_result($id, "collection OUT number", cmp_result(NUM_LST_OUT, $r2.retval, COLL_NUMBER_OUT), NUM_LST_OUT, $r2);
    $db.rollback();
}

sub test_clob_in(int id) {
    my $c = $db.exec("begin qore_test.do_coll_clob(%v, :retval); end;", bindOracleCollection("COL_TEST_CLOB", CLOB_LST_IN));
    #print_rslt($id, CLOB_LST_IN, $c);
    print_result($id, "collection IN clob", cmp_result(NOTHING, $c.retval, COLL_CLOB_IN), CLOB_LST_IN, $c.retval);
    $db.rollback();
}

sub test_clob_out(int id) {
    my $r2 = $db.exec("begin qore_test.get_coll_clob(:retval); end;", placeholderOracleCollection("COL_TEST_clob"));
    #print_rslt($id, CLOB_LST_OUT, $r2);
    print_result($id, "collection OUT clob", cmp_result(CLOB_LST_OUT, $r2.retval, COLL_CLOB_OUT), CLOB_LST_OUT, $r2);
    $db.rollback();
}

sub test_date_in(int id) {

	my list $inp = ($c_date, $c_date, NULL, NOTHING, $c_date);
    my $c = $db.exec("begin qore_test.do_coll_date(%v, :retval); end;", bindOracleCollection("COL_TEST_date", $inp));
    #print_rslt($id, DATE_LST_IN, $c);
    print_result($id, "collection IN date", cmp_result(NOTHING, $c.retval, COLL_DATE_IN), DATE_LST_IN, $c.retval);
    $db.rollback();
}

sub test_date_out(int id) {
    my $r3 = $db.exec("begin qore_test.get_coll_date(:retval); end;", placeholderOracleCollection("COL_TEST_date"));
    #print_rslt($id, DATE_LST_OUT, $r3);
	my list $cmp_list = ($c_date-100D, $c_date, NULL, $c_date+1D);
    print_result($id, "collection OUT date", cmp_result($cmp_list, $r3.retval, COLL_DATE_OUT), $cmp_list, $r3.retval);
    $db.rollback();
}

sub test_object_collection_in(int id) {
    my hash $obj = OBJ_HASH_MAIN + ( "A_OBJECT" : bindOracleObject("TEST_OBJECT_2", OBJ_HASH_SUPP),); 
    my list $colo = bindOracleObject("TEST_OBJECT", $obj), bindOracleObject("TEST_OBJECT", $obj), bindOracleObject("TEST_OBJECT", $obj);
    my $d = $db.exec("begin qore_test.do_coll_obj(%v, :retval); end;", bindOracleCollection("COL_TEST_obj", $colo));
	#print_rslt($id, $colo, $d);
    print_result($id, "collection IN object", cmp_result(NOTHING, $d.retval, COLL_OBJECT_IN), $colo, $d.retval);
    $db.rollback();
}

sub test_object_collection_out(int id) {
    my $r3 = $db.exec("begin qore_test.get_coll_obj(:retval); end;", placeholderOracleCollection("COL_TEST_obj"));
    #print_rslt($id, OBJ_LST_OUT, $r3);
    print_result($id, "collection OUT object", cmp_result(OBJ_LST_OUT, $r3, COLL_OBJECT_OUT), OBJ_LST_OUT, $r3);
    $db.rollback();
}

sub test_string_in(int id) {
    my list $col = ( bindOracleCollection("COL_TEST", (STR_FOO, STR_BAR)), bindOracleCollection("COL_TEST", (STR_FOO , STR_BAR)), bindOracleCollection("COL_TEST", (STR_FOO , STR_BAR)));
    my $c = $db.exec("begin qore_test.do_coll_coll_str(%v, :retval); end;", bindOracleCollection("COL_TEST_coll_str", $col));
    #print_rslt($id, STR_LST_IN, $c);
    print_result($id, "collection IN string", cmp_result(NOTHING, $c.retval, COLL_STRING_IN), STR_LST_IN, $c.retval);
    $db.rollback();
}

sub test_string_out(int id) {
    my $r4 = $db.exec("begin qore_test.get_coll_coll_str(:retval); end;", placeholderOracleCollection("COL_TEST_coll_str"));
    #print_rslt($id, STR_LST_OUT, $r4);
    print_result($id, "collection OUT string", cmp_result(STR_LST_OUT, $r4, COLL_STRING_OUT), STR_LST_OUT, $r4);
    $db.rollback();
}

sub test_objects_in(int id) {
    my list $colco = ( bindOracleCollection("col_test_obj", (bindOracleObject("TEST_OBJECT", OBJ_HASH_MAIN), bindOracleObject("TEST_OBJECT", OBJ_HASH_MAIN))), NULL,
            bindOracleCollection("col_test_obj", (bindOracleObject("TEST_OBJECT", OBJ_HASH_MAIN), NULL, bindOracleObject("TEST_OBJECT", OBJ_HASH_MAIN)))
            );
    my $dco = $db.exec("begin qore_test.do_coll_coll_obj(%v, :retval); end;", bindOracleCollection("COL_TEST_coll_obj", $colco));
    print_result($id, "collection IN objects", cmp_result(NOTHING, $dco.retval, COLL_OBJECTS_IN), $colco, $dco.retval);
    $db.rollback();

}

sub test_objects_out(int id) {
    my $r5 = $db.exec("begin qore_test.get_coll_coll_obj(:retval); end;", placeholderOracleCollection("COL_TEST_coll_obj"));
	my list $cmp_list = (OBJ_LST_OUT, NULL, OBJ_LST_OUT, OBJ_LST_OUT);
	print_result($id, "collection IN objects", cmp_result($cmp_list, $r5.retval, COLL_OBJECTS_IN), $cmp_list, $r5.retval);
    $db.rollback();

}

sub test_timestamp_in(int id) {
    my list $colt = $c_date, $c_date, NULL, NOTHING, $c_date;
    my $d = $db.exec("begin qore_test.do_coll_timestamp_tz(%v, :retval); end;", bindOracleCollection("COL_TEST_timestamp_tz", $colt));
    #printf("collection: %N\n", $d);
    print_result($id, "collection IN timestamp", cmp_result(NOTHING, $d.retval, COLL_TIMESTAMP_IN), TIMESTAMP_LST_IN, $d.retval);
    $db.rollback();
}

sub test_timestamp_out(int id) {
    my $r4 = $db.exec("begin qore_test.get_coll_timestamp_tz(:retval); end;", placeholderOracleCollection("COL_TEST_timestamp_tz"));
    #printf("retval: %N\n", $r4);
    my list $cmp_list = ($c_date-100D, $c_date, NULL, $c_date+1D);
    print_result($id, "collection OUT timestamp", cmp_result($cmp_list, $r4.retval, COLL_TIMESTAMP_OUT), $cmp_list, $r4.retval);
    $db.rollback();
}

sub test_exception1(int id) {
    try {
        my hash $obj =  OBJ_HASH_MAIN + ( "A_OBJECT" : bindOracleObject("TEST_OBJECT_2", OBJ_HASH_SUPP),);
        my $r = $db.exec("begin qore_test.do_obj(%v, :retval); end;",
                $obj, # missing bindOracleObject
                Type::String);
        $db.rollback();
        print_result($id, "object is not bound correctly", Qore::False, "missing bind oracle object for the api db.exec()", $r);
    }
    catch ($ex) {
        print_result($id, "object is not bound correctly", Qore::True, "missing bind oracle object for the api db.exec()", $ex);
    }
}

sub test_exception2(int id) {
    try {
        my hash $obj = OBJ_HASH_MAIN + ( "A_OBJECT" : OBJ_HASH_SUPP,);
        my $r = $db.exec("begin qore_test.do_obj(%v, :retval); end;",
                bindOracleObject("TEST_OBJECT", $obj), # missing bindOracleObject
                Type::String);
        $db.rollback();
        print_result($id, "object(sub/internal) is not bound correctly", Qore::False, "missing bind oracle object for the api db.exec()", $r);
    }
    catch ($ex) {
        print_result($id, "object(sub/internal) is not bound correctly", Qore::True, "missing bind oracle object for the api db.exec()", $ex);
    }
}

sub test_exception3(int id) {
    try {
        my list $col = STR_FOO, STR_BAR,NULL, NOTHING, STR_END;
        my $r = $db.exec("begin qore_test.do_coll(%v, :retval); end;",
                $col, # missing bindOracleObject
                Type::String);
        $db.rollback();
        print_result($id, "collection is not bound correctly", Qore::False, "missing bind oracle object for the api db.exec()", $r);
    }
    catch ($ex) {
        print_result($id, "collection is not bound correctly", Qore::True, "missing bind oracle object for the api db.exec()", $ex);
    }
}

sub test_exception4(int id) {
    try {
        my list $colcs = ( ( STR_FOO, STR_BAR ),  );
        my $dcs = $db.exec("begin qore_test.do_coll_coll_str(%v, :retval); end;", bindOracleCollection("COL_TEST_coll_str", $colcs));
        $db.rollback();
        print_result($id, "collection(sub/internal) is not bound correctly", Qore::False, "type 'list' is not supported in binaOracleCollection()", $r);
    }
    catch ($ex) {
        print_result($id, "collection(sub/internal) is not bound correctly", Qore::True, "type 'list' is not supported in binaOracleCollection()", $ex);
    }
}

sub test_object_insert(int id) {

	on_success $db.commit();
	on_error $db.rollback();
	
	try {	
		my $rins = $db.exec("insert into t_object_test values (%v, %v, %v)",
				1,
				bindOracleObject("TEST_tab_OBJECT", OBJ_HASH_IN),
				bindOracleCollection("TEST_tab_coll", (NUM_INT, NUM_INT, NULL, NOTHING, NUM_INT,))
				);
		if($rins == 1) 
			print_result($id, "object direct insert", Qore::True, "object insert to db succesfull", $rins);
		else
			print_result($id, "object direct insert", Qore::False, "object insert to db unsuccesfull", $rins);

	} catch ($ex) {
			print_result($id, "object direct insert", Qore::False, "object insert to db unsuccesfull", $ex);
	}
	printf("rins: %N\n", $rins);

}

/*
sub test_object_select() {

    printf("\nOBJECT direct select\n");
    my $rsel = $db.select("select * from t_object_test where rownum < 5");
    $rsel = $db.select("select t.* from t_object_test t");
    printf("rsel: %N\n", $rsel);

}
*/

sub print_rslt(int id, any input, any output) {
    printf("\n%d\n", $id);
    printf("input == %N\n", $input);
    printf("output == %N\n", $output);
    printf("done\n");
}
