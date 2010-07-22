CREATE OR REPLACE PACKAGE BODY qore_test
is

procedure do_obj (
    o in test_object,
    retval out varchar2
)
is
begin
    retval := o.a_number || '-'
           || o.a_text || '-'
           || o.a_textc || '-'
           || o.a_clob;
end do_obj;

procedure get_obj (
    o out test_object
)
is
begin
    o := test_object(a_text => dbms_random.string('l', 10),
                     a_number => dbms_random.value(1, 999),
                     a_textc => dbms_random.string('l', 10),
                     a_clob => dbms_random.string('l', 1000));
end get_obj;

procedure do_coll (
    c in col_test,
    retval out varchar2
)
is
begin
    retval := '|';
    for i in 1 .. c.count loop
        retval := retval || i || '=' || nvl(c(i), '{NULL}') ||'|';
    end loop;
end do_coll;

procedure get_coll (
    c out col_test
)
is
begin
    c := col_test(
            dbms_random.string('l', 10),
            dbms_random.string('l', 100),
            null,
            dbms_random.string('l', 1)
            );
end get_coll;

procedure do_coll_num (
    c in col_test_num,
    retval out varchar2
)
is
begin
    retval := '|';
    for i in 1 .. c.count loop
        retval := retval || i || '=' || c(i) ||'|';
    end loop;
end do_coll_num;

procedure get_coll_num (
    c out col_test_num
)
is
begin
    c := col_test_num(
            trunc(100*dbms_random.value),
            trunc(100*dbms_random.value),
            null,
            trunc(100*dbms_random.value)
            );
end get_coll_num;

procedure do_coll_clob (
    c in col_test_clob,
    retval out varchar2
)
is
begin
    retval := '|';
    for i in 1 .. c.count loop
        retval := retval || i || '=' || c(i) ||'|';
    end loop;
end do_coll_clob;

procedure get_coll_clob (
    c out col_test_clob
)
is
begin
    c := col_test_clob(
            dbms_random.string('l', 1000),
            dbms_random.string('l', 1000),
            null,
            dbms_random.string('l', 1000)
            );
end get_coll_clob;

--procedure get_records (
--    cnt in number,
--    tab out t_table_record
--    )
--is
--begin
--    --
--    if cnt < 1 then
--        raise_application_error(-20001, 'CNT has to be greater than 0');
--    end if;
--    --
--    for i in 1 .. cnt loop
--        tab(i).num := dbms_random.value(1,100);
--        tab(i).var := dbms_random.string('P', 20);
--    end loop;
--    --
--end get_records;
--
--procedure do_records (
--    tab in t_table_record,
--    cnt out number
--)
--is
--begin
--    --
--    for i in tab.first .. tab.last loop
--        dbms_output.put_line('num=' || to_char(tab(i).num) || chr(9)
--                             || 'var=' || tab(i).var);
--    end loop;
--    --
--    cnt := tab.count;
--    --
--end do_records;
--
--
--procedure get_rowtypes (
--    cnt in number,
--    tab out t_table_rowtype
--    )
--is
--begin
--    --
--    if cnt < 1 then
--        raise_application_error(-20001, 'CNT has to be greater than 0');
--    end if;
--    --
--    open user_objs;
--    fetch user_objs bulk collect into tab limit cnt;
--    close user_objs;
--    --
--end get_rowtypes;
--
--procedure do_rowtypes (
--    tab in t_table_rowtype,
--    cnt out number
--)
--is
--begin
--    --
--    for i in tab.first .. tab.last loop
--        dbms_output.put_line(tab(i).object_name || chr(9)
--                             || tab(i).object_type);
--    end loop;
--    --
--    cnt := tab.count;
--    --
--end do_rowtypes;
--
--
--procedure get_objects (
--    cnt in number,
--    tab out t_table_object
--)
--is
--begin
--    --
--    if cnt < 1 then
--        raise_application_error(-20001, 'CNT has to be greater than 0');
--    end if;
--    --
--    tab := t_table_object();
--    tab.extend(cnt);
--    for i in 1 .. cnt loop
--        tab(i) := test_object(t => dbms_random.string('l', 10),
--                              n => dbms_random.value(1, 999));
--    end loop;
--    --
--end get_objects;
--
--procedure do_objects (
--    tab in t_table_object,
--    cnt out number
--)
--is
--    o test_object;
--begin
--    --
--    for i in tab.first .. tab.last loop
--        o := tab(i);
--        o.printout;
--    end loop;
--    --
--    cnt := tab.count;
--    --
--end do_objects;
--
--
end qore_test;
/