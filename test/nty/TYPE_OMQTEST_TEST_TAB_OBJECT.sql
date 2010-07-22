CREATE OR REPLACE TYPE test_tab_object
    authid current_user
as object (
    
    a_text varchar2(10),
    a_number number
)
/


CREATE OR REPLACE TYPE test_tab_coll as table of number
/

