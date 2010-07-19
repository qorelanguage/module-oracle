%requires oracle
printf("\nQore named types test\n\n");

my $db = new Datasource("oracle", "omqtest", "omqtest", "stimpy11");
$db.open();

printf("OBJECT IN\n");
my hash $obj = ("A_TEXT": "1",
                "A_NUMBER": 2,
                "A_NUM_1": 1,
                "A_TEXTC" : "foo bar",
                "A_CLOB" : "lorem ipsum clob sir amet",
                );

my $r = $db.exec("begin qore_test.do_obj(%v, :retval); end;",
                 bindOracleObject("OMQTEST.TEST_OBJECT", $obj),
                 Type::String);
printf("object: %N\n", $r);
$db.rollback();

printf("\nOBJECT OUT\n");
my $r1 = $db.exec("begin qore_test.get_obj(:retval); end;", placeholderOracleObject("TEST_OBJECT"));
printf("retval: %N\n", $r1);
$db.rollback();



printf("\nCOLLECTION IN varchar2\n");
my list $col = 'foo', 'bar',NULL, NOTHING, "the end";
my $c = $db.exec("begin qore_test.do_coll(%v, :retval); end;",
                 bindOracleCollection("COL_TEST", $col));
printf("collection: %N\n", $c);
$db.rollback();

printf("\nCOLLECTION OUT varchar2\n");
my $r2 = $db.exec("begin qore_test.get_coll(:retval); end;", placeholderOracleCollection("COL_TEST"));
printf("retval: %N\n", $r2);
$db.rollback();


printf("\nCOLLECTION IN number\n");
my list $col = 2, 213,NULL, NOTHING, 666;
my $c = $db.exec("begin qore_test.do_coll_num(%v, :retval); end;",
                 bindOracleCollection("COL_TEST_NUM", $col));
printf("collection: %N\n", $c);
$db.rollback();

printf("\nCOLLECTION OUT number\n");
my $r2 = $db.exec("begin qore_test.get_coll_num(:retval); end;", placeholderOracleCollection("COL_TEST_NUM"));
printf("retval: %N\n", $r2);
$db.rollback();


printf("\nCOLLECTION IN clob\n");
my list $col = 'foo', 'bar',NULL, NOTHING, "the end";
my $c = $db.exec("begin qore_test.do_coll_clob(%v, :retval); end;",
                 bindOracleCollection("COL_TEST_CLOB", $col));
printf("collection: %N\n", $c);
$db.rollback();

printf("\nCOLLECTION OUT clob\n");
my $r2 = $db.exec("begin qore_test.get_coll_clob(:retval); end;", placeholderOracleCollection("COL_TEST_clob"));
printf("retval: %N\n", $r2);
$db.rollback();
