CREATE OR REPLACE TYPE test_tab_object
    authid current_user
as object (
    
    a_text varchar2(10),
    a_number number
)
/


CREATE OR REPLACE TYPE test_tab_coll as table of number
/

CREATE OR REPLACE TYPE test_object_timestamp    
    authid current_user
as object (

    a_date date,
    a_tstamp timestamp,
    a_tstamp_tz timestamp with time zone,
    a_int_ym interval year to month,
    a_int_ds interval day to second
)
/