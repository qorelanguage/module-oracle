%requires oracle
printf("\nQore named types test - various errors checking\n\n");

my $db = new Datasource("oracle", "omqtest", "omqtest", "stimpy11");
$db.open();


try {
    printf("\nObject is not bound correctly\n");
    my hash $obj = ("A_TEXT": "1",
                    "A_NUMBER": 2,
                    "A_NUM_1": 1,
                    "A_TEXTC" : "foo bar",
                    "A_CLOB" : "lorem ipsum clob sir amet",
                    "A_DATE" : now_ms(),
                    "A_TSTAMP" : now_ms(),
                    "A_TSTAMP_TZ" : now_ms(),
                    "A_OBJECT" : bindOracleObject("TEST_OBJECT_2", ("TEXT2" : "foobar", "NUMBER2" : 666))
                    );

    my $r = $db.exec("begin qore_test.do_obj(%v, :retval); end;",
                     $obj, # missing bindOracleObject
                     Type::String);
    printf("object: %N\n", $r);
    $db.rollback();
}
catch ($ex) {
    printf("%N\n", $ex);
}


try {
    printf("\nObject (sub/internal) is not bound correctly\n");
    my hash $obj = ("A_TEXT": "1",
                    "A_NUMBER": 2,
                    "A_NUM_1": 1,
                    "A_TEXTC" : "foo bar",
                    "A_CLOB" : "lorem ipsum clob sir amet",
                    "A_DATE" : now_ms(),
                    "A_TSTAMP" : now_ms(),
                    "A_TSTAMP_TZ" : now_ms(),
                    "A_OBJECT" : ("TEXT2" : "foobar", "NUMBER2" : 666)
                    );

    my $r = $db.exec("begin qore_test.do_obj(%v, :retval); end;",
                     bindOracleObject("TEST_OBJECT", $obj), # missing bindOracleObject
                     Type::String);
    printf("object: %N\n", $r);
    $db.rollback();
}
catch ($ex) {
    printf("%N\n", $ex);
}
