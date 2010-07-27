CREATE OR REPLACE PACKAGE         qore_test  
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

-- date collection test
procedure do_coll_date (
    c in col_test_date,
    retval out varchar2
);
procedure get_coll_date (
    c out col_test_date
);

-- object collection test
procedure do_coll_obj (
    c in col_test_obj,
    retval out varchar2
);
procedure get_coll_obj (
    c out col_test_obj
);

-- collections of strings collection test
procedure do_coll_coll_str (
    c in col_test_coll_str,
    retval out varchar2
);
procedure get_coll_coll_str (
    c out col_test_coll_str
);

-- collections of objects collection test
procedure do_coll_coll_obj (
    c in col_test_coll_obj,
    retval out varchar2
);
procedure get_coll_coll_obj (
    c out col_test_coll_obj
);

end qore_test;
/