CREATE OR REPLACE PACKAGE BODY         qore_test       
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
           || o.a_clob || '-'
           || o.a_date;
--null;
end do_obj;

procedure get_obj_2 (
    o out test_object_2
)
is
begin
    o := test_object_2(text2 => dbms_random.string('l', 10),
                     number2 => dbms_random.value(1, 999)
                     );
end get_obj_2;

procedure get_obj (
    o out test_object
)
is
    tmp_o2 test_object_2;
begin
    dbms_random.seed( dbms_random.value(1, 999) );
    get_obj_2(tmp_o2);
    o := test_object(a_text => dbms_random.string('l', 10),
                     a_number => dbms_random.value(1, 999),
                     a_textc => dbms_random.string('l', 10),
                     a_clob => dbms_random.string('l', 1000),
                     a_date => sysdate,
--                     , a_tstamp => current_timestamp
--                     , a_tstamp_tz => current_timestamp
                     a_object => tmp_o2
                     );
end get_obj;

procedure do_obj_timestamp (
    o in test_object_timestamp,
    retval out varchar2
)
is
begin
    retval := o.a_date || '|'
           || o.a_tstamp || '|'
           || o.a_tstamp_tz || '|'
--           || o.a_int_ym || '|'
--           || o.a_int_ds
            ;
--null;
end do_obj_timestamp;


procedure get_obj_timestamp (
    o out test_object_timestamp
)
is
begin
    o := test_object_timestamp(
                     a_date => sysdate,
                     a_tstamp => current_timestamp,
                     a_tstamp_tz => current_timestamp,
                     a_int_ym => INTERVAL '11' MONTH,
                     a_int_ds => INTERVAL '333' SECOND
                     );
end get_obj_timestamp;

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

procedure do_coll_date (
    c in col_test_date,
    retval out varchar2
)
is
begin
    retval := '|';
    for i in 1 .. c.count loop
        retval := retval || i || '=' || c(i) ||'|';
    end loop;
end do_coll_date;

procedure get_coll_timestamp_tz (
    c out col_test_timestamp_tz
)
is
begin
    c := col_test_timestamp_tz(
            sysdate-100,
            current_timestamp,
            null,
            current_timestamp+1
            );
end get_coll_timestamp_tz;

procedure do_coll_timestamp_tz (
    c in col_test_timestamp_tz,
    retval out varchar2
)
is
begin
    retval := '|';
    for i in 1 .. c.count loop
        retval := retval || i || '=' || c(i) ||'|';
    end loop;
end do_coll_timestamp_tz;

procedure get_coll_date (
    c out col_test_date
)
is
begin
    c := col_test_date(
            sysdate-100,
            sysdate,
            null,
            sysdate+1
            );
end get_coll_date;

procedure do_coll_obj (
    c in col_test_obj,
    retval out varchar2
)
is
    tmpstr varchar2(200);
begin
    retval := '|';
    for i in 1 .. c.count loop
        do_obj(c(i), tmpstr);
        retval := retval || i || '=' || tmpstr ||'|';
    end loop;
end do_coll_obj;

procedure get_coll_obj (
    c out col_test_obj
)
is
    tmp1 test_object;
    tmp2 test_object;
    tmp3 test_object;
begin
    get_obj(tmp1);
    get_obj(tmp2);
    get_obj(tmp3);
    c := col_test_obj(
            tmp1,
            null,
            tmp2,
            tmp3
            );
end get_coll_obj;

procedure do_coll_coll_str (
    c in col_test_coll_str,
    retval out varchar2
)
is
    tmp col_test;
begin
    for i in 1 .. c.count loop
        retval := retval || chr(10); 
        if c(i) is null then
            retval := retval || '{NULL}';
            continue;
        end if;
        tmp := c(i);
        for j in 1 .. tmp.count loop
            retval := retval || j || '=' || tmp(j) ||'|';
        end loop;
    end loop;
end do_coll_coll_str;

procedure get_coll_coll_str (
    c out col_test_coll_str
)
is
    tmp1 col_test;
    tmp2 col_test;
    tmp3 col_test;
begin
    get_coll(tmp1);
    get_coll(tmp2);
    get_coll(tmp3);
    c := col_test_coll_str(
            tmp1,
            null,
            tmp2,
            tmp3
            );
end get_coll_coll_str;

procedure do_coll_coll_obj (
    c in col_test_coll_obj,
    retval out varchar2
)
is
    tmpstr varchar2(2000);
begin
    for i in 1 .. c.count loop
        if c(i) is null then
            tmpstr := '{NULL}';
        else
            do_coll_obj(c(i), tmpstr);
        end if;
        retval := retval || chr(10) || tmpstr; 
    end loop;
end do_coll_coll_obj;

procedure get_coll_coll_obj (
    c out col_test_coll_obj
)
is
    tmp1 col_test_obj;
    tmp2 col_test_obj;
    tmp3 col_test_obj;
begin
    get_coll_obj(tmp1);
    get_coll_obj(tmp2);
    get_coll_obj(tmp3);
    c := col_test_coll_obj(
            tmp1,
            null,
            tmp2,
            tmp3
            );
end get_coll_coll_obj;


/*
 * PL/SQL types **********************************************
 */

procedure get_records (
    cnt in number,
    tab out t_table_record
    )
is
begin
    --
    if cnt < 1 then
        raise_application_error(-20001, 'CNT has to be greater than 0');
    end if;
    --
    for i in 1 .. cnt loop
        tab(i).num := dbms_random.value(1,100);
        tab(i).var := dbms_random.string('P', 20);
    end loop;
    --
end get_records;

procedure do_records (
    tab in t_table_record,
    cnt out number
)
is
begin
    --
    for i in tab.first .. tab.last loop
        dbms_output.put_line('num=' || to_char(tab(i).num) || chr(9)
                             || 'var=' || tab(i).var);
    end loop;
    --
    cnt := tab.count;
    --
end do_records;


procedure get_rowtypes (
    cnt in number,
    tab out t_table_rowtype
    )
is
begin
    --
    if cnt < 1 then
        raise_application_error(-20001, 'CNT has to be greater than 0');
    end if;
    --
    open user_objs;
    fetch user_objs bulk collect into tab limit cnt;
    close user_objs;
    --
end get_rowtypes;

procedure do_rowtypes (
    tab in t_table_rowtype,
    cnt out number
)
is
begin
    --
    for i in tab.first .. tab.last loop
        dbms_output.put_line(tab(i).object_name || chr(9)
                             || tab(i).object_type);
    end loop;
    --
    cnt := tab.count;
    --
end do_rowtypes;
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