CREATE OR REPLACE PACKAGE qore_test
is

-- object tests
procedure do_obj (
    o in test_object,
    retval out varchar2
);

procedure get_obj (
    o out test_object
);

-- varchar2 collection test
procedure do_coll (
    c in col_test,
    retval out varchar2
);

procedure get_coll (
    c out col_test
);

-- number collection test
procedure do_coll_num (
    c in col_test_num,
    retval out varchar2
);

procedure get_coll_num (
    c out col_test_num
);

-- clob collection test
procedure do_coll_clob (
    c in col_test_clob,
    retval out varchar2
);
procedure get_coll_clob (
    c out col_test_clob
);


end qore_test;
/

