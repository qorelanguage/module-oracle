CREATE OR REPLACE TYPE test_object
    authid current_user
as object (
    
    a_text varchar2(10),
    a_number number/*(3)*/,
--    a_num_1 number(4,5),
    a_textc char(10),
    a_clob clob,
    a_date date,
    a_tstamp timestamp,
    a_tstamp_tz timestamp with time zone,
--    a_object test_object,
--    a_table qore_test.t_table_rowtype%type,
    
    constructor function test_object(t in varchar2, n in number)
        return self as result

--    member procedure printout
)
/

