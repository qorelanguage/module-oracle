
%requires oracle

const TYPE_NUMBER = 0;
const TYPE_STRING = 1;
const TYPE_OBJECT = 2;

const NUM_1_999 = 898; 
const NUM_INT = 456;
const NUM_DBL = 3.1415927;

const STR_1 = "c";
const STR_10 = "nzsuxobymr";
const STR_100 = "ziafgnjvmiijgezxivssbooduhfkzdschhwnpopcimbxrnxaqqhcnavuqfpfrbyuqfwvspaqiuptblelxmnmeoxxnmthqnwzrysk";
const STR_1000 = "hazmsuxjfhiqefmoxnumctthzjqzksnmegkweyrmryjhkpreejwvkchhjpmcnkvkosmiddapleunjlletmvuuutowwzrjmxfqwqotstqoqjunslcbhxifpzkpmanqyvcblcdeeuhdeoimeryexzocwqgjrckrospclxsxfkfmmqstqiujabsulikceovtmxfqmajkcdrjqgvnzasrddlabaeqlnqsikcyrcdtmlsqioiljduirguoykgxo";
const STR_DATE = "11-SEP-14";
const STR_FDATE = "2014-09-11 00:00:00.000000 Thu +02:00 (CEST)";
const STR_DATE_PLUS_1 = "2014-09-12 00:00:00.000000 Fri +02:00 (CEST)";
const STR_DATE_MINUS_100 = "2014-06-03 00:00:00.000000 Tue +02:00 (CEST)";
const TIMESTAMP_STR = "11-SEP-14 12.00.00.000000 AM +02:00";

our date $c_date =  date("2014-09-11");

const STR_FOO = "foo";
const STR_BAR = "bar";
const STR_END = "end";
const STR_CLOB = "lorem ipsum clob sir amet";
const STR_NULL = "{NULL}";

const OBJ_SUPP = (STR_FOO, NUM_1_999);

const OBJ_HASH_MAIN = ( "A_TEXT"  : STR_1,
                        "A_NUMBER": NUM_INT,
                        "A_TEXTC" : STR_FOO,
                        "A_CLOB"  : STR_CLOB,
                        "A_DATE"  : STR_DATE,
                        );
const OBJ_HASH_MAIN_IN = (STR_1, NUM_INT, STR_FOO, STR_CLOB, "01-JAN-70");

const OBJ_HASH_SUPP = ("TEXT2" : STR_FOO, "NUMBER2" : NUM_1_999);

const OBJ_HASH_IN = ( "A_TEXT"   : STR_1,
                      "A_NUMBER" : NUM_INT,
                      "A_TEXTC"  : STR_FOO,
                      "A_CLOB"   : STR_CLOB, 
                      "A_DATE"   : STR_FDATE, 
                      "A_OBJECT" : ("TEXT2"  : STR_10,
                                    "NUMBER2": NUM_1_999,),
                      );

const VARCHAR2_LST_IN = (STR_FOO, STR_BAR, NULL, NOTHING, STR_END);
const VARCHAR2_LST_OUT = (STR_10, STR_100, NULL, STR_1);
const NUM_LST_IN = (NUM_INT,  NUM_INT, NULL, NOTHING, NULL, "456", 456n, NUM_INT);
const NUM_LST_OUT = (NUM_INT, NUM_INT, NULL, NUM_INT);
const CLOB_LST_IN = (STR_FOO, STR_BAR, NULL, NOTHING, STR_END);
const CLOB_LST_OUT = (STR_100, STR_100, NULL, STR_100);
const DATE_LST_IN = (STR_DATE, STR_DATE, NULL, NOTHING, STR_DATE);
const DATE_LST_OUT = (STR_DATE_MINUS_100, STR_FDATE, NULL, STR_DATE_PLUS_1);
const OBJ_LST_OUT = (OBJ_HASH_IN, NULL, OBJ_HASH_IN, OBJ_HASH_IN);
const STR_LST_IN = ((STR_FOO, STR_BAR), (STR_FOO, STR_BAR), (STR_FOO, STR_BAR));  
const STR_LST_OUT =(VARCHAR2_LST_OUT, NULL, VARCHAR2_LST_OUT, VARCHAR2_LST_OUT);
#const OBJS_LST_IN = ();
#const OBJS_LST_OUT = ();
const TIMESTAMP_LST_IN = (TIMESTAMP_STR, TIMESTAMP_STR, NULL, NOTHING, TIMESTAMP_STR);

const COL_WIDTH = 100;
const OFFSET = 5;

const COLL_VARCHAR2_OUT  = 0;
const COLL_VARCHAR2_IN   = 1;
const COLL_NUMBER_IN     = 2;
const COLL_NUMBER_OUT    = 3;
const COLL_CLOB_IN       = 4;
const COLL_CLOB_OUT      = 5;
const COLL_DATE_IN       = 6;
const COLL_DATE_OUT      = 7;
const COLL_OBJECT_IN     = 8;
const COLL_OBJECT_OUT    = 9;
const COLL_STRING_IN     = 10;
const COLL_STRING_OUT    = 11;
const COLL_OBJECTS_IN    = 12;
const COLL_OBJECTS_OUT   = 13;
const COLL_TIMESTAMP_IN  = 14;
const COLL_TIMESTAMP_OUT = 15;


our int $success_count = 0;
our string $err_msg;


sub print_details(int level, string type, any data, *hash support) {
    switch ($level) {
        case 0 :
            printf("%s\n", $type);
            printf("%s\n", $support.in_str);
            break;
        case 1 :
            printf("%s\n", $type);
            printf("data : \n%N\n", $data);
            printf("more :\n");
            printf("string : \n%s\n", $support.in_str);
            break;
        case 2 :
            printf("%s\n", $type);
            printf("%y\n", $data);
        default : #do nothing
    }
}

sub print_char(string c, int width) {
    for(int $i=0; $i<$width; $i++) {
        printf("%s", $c);
    }
}

sub print_row(list row_data) {
 
    print_char('-', COL_WIDTH);
    printf(" -----------\n");
    printf("%s ", $row_data[0]);
    print_char(' ', COL_WIDTH + OFFSET - $row_data[0].length());
    printf(" %s\n\n", $row_data[1]);

}

sub print_result(int id, string msg, bool result, any input, any output) {
    
    if ($result) {
        $success_count++;
        print_char('-', COL_WIDTH);
        printf(" -----------\n");
        printf("TEST-%d: %s ", $id, $msg);
        print_char(' ', COL_WIDTH - OFFSET - $msg.length());
        printf(" PASS\n");
    } else {
        print_char('-', COL_WIDTH);
        printf(" -----------\n");
        printf("TEST-%d: %s ", $id, $msg);
        print_char(' ', COL_WIDTH - OFFSET - $msg.length());
        printf(" FAIL\n");
        printf("\n");
		printf("%s\n", $msg);
        print_details(2, "INPUT", $input);
        print_details(2, "OUTUT", $output);
    }
}

bool sub cmp_result(any $input, any $output, int type) {

	if(!exists $input) {	
		my string $cmp_str = get_cmp_str($type);	
		if(exists $cmp_str) {
			return $cmp_str == $output;
		}
		return $input == $output;
	}
	else if($output.type() == "list") {
        if($input.size() != $output.size()) {
			sprintf($msg, "[ERROR] Input and output size doen't match : DETAILS input size==[%d] output size==[%d]", $input.size(), $output.size());
			return Qore::False;
		}
        for(my int $i=0; $i<$input.size(); $i++){
           if($input[$i] != $output[$i]) {
				printf("%s, %s\n", $input[$i].type(), $output[$i].type());
                sprintf($msg, "[ERROR] Results doesn't match : DETAILS: input==[%s] output==[%s]\n", $input[$i], $ouput[$i]);
                return Qore::False;
           }
        }
        return Qore::True;
    } 
    else if($output.type() == "hash") {
        return Qore::True; 
    } 
	return $input == $output;

}

sub get_cmp_str(int v_type) {

    switch ($v_type) {
        case COLL_VARCHAR2_IN   : return "|" + create_cmp_str_1(VARCHAR2_LST_IN, TYPE_STRING);
        case COLL_NUMBER_IN     : return "|" + create_cmp_str_1(NUM_LST_IN, TYPE_NUMBER);
        case COLL_CLOB_IN       : return "|" + create_cmp_str_1(CLOB_LST_IN, TYPE_NUMBER);
        case COLL_DATE_IN       : return "|" + create_cmp_str_1(DATE_LST_IN, TYPE_NUMBER);
        case COLL_OBJECT_IN     :   
									my string $cmp_str = "|";
									for(my int $i=0; $i<3; $i++){
										$cmp_str += ($i+1) + "=" +create_cmp_str_1(OBJ_HASH_MAIN_IN, TYPE_OBJECT);
									}
									return $cmp_str;
        case COLL_STRING_IN     : return create_cmp_str_1(STR_LST_IN, TYPE_STRING);
        case COLL_OBJECTS_IN    : 
									my string $cmp_str = "\n|";
									for($i=0; $i<2; $i++){
										$cmp_str += ($i+1) + "=" + create_cmp_str_1(OBJ_HASH_MAIN_IN, TYPE_OBJECT);
									}
									$cmp_str += "\n{NULL}\n|";
									for($i=0; $i<3; $i++){
										$cmp_str += ($i+1) + "=" + create_cmp_str_1(OBJ_HASH_MAIN_IN, TYPE_OBJECT);
									}
									return $cmp_str;
        case COLL_TIMESTAMP_IN  : return "|" + create_cmp_str_1(TIMESTAMP_LST_IN, TYPE_NUMBER);
		default : return NOTHING;
    }

}

string sub getKeyValuePair(string key, any val, int v_type) {

	if($v_type == TYPE_STRING) {
		if(!exists $val) $val = "{NULL}";
		else if($val == NULL) $val = "{NULL}";
	}	 

	return $key + "=" + $val + "|";

}


sub create_cmp_str_1(list col, int v_type) {

    my string $ret_str = "";

	if($v_type == TYPE_OBJECT) {
		$ret_str = $col[0];
		for(my int $j=1; $j<$col.size(); $j++) {
			my any $val = $col[$j];
			$ret_str += "-" + $val;
		}
		$ret_str += "|";
	}
	else {
		for(my int $i=0; $i<$col.size(); $i++){
			if($col[$i].type() == "list"){
				if($ret_str == "|") $ret_str = "\n";
				else $ret_str += "\n";
				for(my int $j=0; $j<$col[$i].size(); $j++){
					$ret_str += getKeyValuePair(($j+1) + "", $col[$i][$j], TYPE_STRING);
				}
			} else {
				$ret_str += getKeyValuePair(($i+1) + "", $col[$i], $v_type);
			}
		}
	}
    return $ret_str;
}
