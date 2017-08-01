CREATE OR REPLACE TYPE clob_test_object
    authid current_user
as object (
    a_clob clob
)
/

CREATE OR REPLACE TYPE clob_test_collection as table of clob
/

----------------------------------------------------------------------

CREATE OR REPLACE PACKAGE clob_test
is

procedure get_clob_object (
    i in varchar2,
    o out clob_test_object
);
procedure get_clob_collection (
    i in varchar2,
    o out clob_test_collection
);

end clob_test;
/

----------------------------------------------------------------------

CREATE OR REPLACE PACKAGE BODY clob_test
is


procedure get_clob_object (
    i in varchar2,
    o out clob_test_object
)
is
begin
    o := clob_test_object(a_clob => i);
end get_clob_object;


procedure get_clob_collection (
    i in varchar2,
    o out clob_test_collection
)
is
begin
    o := clob_test_collection(i, i, null, i);
end get_clob_collection;


end clob_test;
/
