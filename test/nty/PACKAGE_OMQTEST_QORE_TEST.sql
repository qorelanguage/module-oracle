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

-- time in objs
procedure do_obj_timestamp (
    o in test_object_timestamp,
    retval out varchar2
);
procedure get_obj_timestamp (
    o out test_object_timestamp
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

-- timestamp (tz) collection test
procedure get_coll_timestamp_tz (
    c out col_test_timestamp_tz
);
procedure do_coll_timestamp_tz (
    c in col_test_timestamp_tz,
    retval out varchar2
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

/*
 * PL/SQL types ***************************************************
 */

-- table of...
type t_record is record (
        num number(3),
        var varchar2(20)
);

type t_table_record is table of t_record index by binary_integer;


-- rowtype
cursor user_objs is
        select object_name, object_type, last_ddl_time
        from user_objects;

type t_table_rowtype is table of user_objs%rowtype;


-- procedures to test
procedure get_records (
    cnt in number,
    tab out t_table_record
);

procedure do_records (
    tab in t_table_record,
    cnt out number
);

procedure get_rowtypes (
    cnt in number,
    tab out t_table_rowtype
);

procedure do_rowtypes (
    tab in t_table_rowtype,
    cnt out number
);

--procedure get_objects (
--    cnt in number,
--    tab out t_table_object
--);
--
--procedure do_objects (
--    tab in t_table_object,
--    cnt out number
--);

end qore_test;
/