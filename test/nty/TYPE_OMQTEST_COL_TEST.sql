CREATE OR REPLACE TYPE col_test as table of varchar2(100);
/
CREATE OR REPLACE TYPE col_test_date as table of date
/
CREATE OR REPLACE TYPE col_test_coll_obj as table of col_test_obj
/
CREATE OR REPLACE TYPE col_test_coll_str as table of col_test
/
CREATE OR REPLACE TYPE col_test_obj  as table of test_object
/
CREATE OR REPLACE TYPE col_test_timestamp_tz as table of timestamp with time zone
/