begin
    execute immediate 'drop table t_object_test';
exception when others then null;
end;
/

CREATE TABLE t_object_test
(
    id                               NUMBER                          
  , obj                              TEST_TAB_OBJECT                 
  , coll                             TEST_TAB_COLL
)
NESTED TABLE coll STORE AS coll_tab
;

