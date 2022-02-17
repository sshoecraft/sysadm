
#include "gendb.h"

extern int opts;
#define MYSQL 0

static void write_select(FILE *fp, TABLE *table, int decl) {
	fprintf(fp,"function do_read() {\n");
	fprintf(fp,"\tglobal $db_conn,$debug,$fields;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\tif (isset($_REQUEST[\"sort\"]))\n");
	fprintf(fp,"\t\t$sort = $_REQUEST[\"sort\"];\n");
	fprintf(fp,"\telse\n");
	fprintf(fp,"\t\t$sort = \"\";\n");
	fprintf(fp,"\tif (isset($_REQUEST[\"dir\"]))\n");
	fprintf(fp,"\t\t$dir = $_REQUEST[\"dir\"];\n");
	fprintf(fp,"\telse\n");
	fprintf(fp,"\t\t$dir = \"ASC\";\n");
	fprintf(fp,"\tif (isset($_REQUEST[\"start\"]) && $_REQUEST[\"start\"] != \"\")\n");
	fprintf(fp,"\t\t$start = $_REQUEST[\"start\"];\n");
	fprintf(fp,"\telse\n");
	fprintf(fp,"\t\t$start = -1;\n");
	fprintf(fp,"\tif (isset($_REQUEST[\"limit\"]))\n");
	fprintf(fp,"\t\t$limit = $_REQUEST[\"limit\"];\n");
	fprintf(fp,"\telse\n");
	fprintf(fp,"\t\t$limit = -1;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\t$query = \"SELECT \";\n");
	fprintf(fp,"\t$i = 0;\n");
	fprintf(fp,"\tforeach(array_keys($fields) as $field_name) {\n");
	fprintf(fp,"\t\tif ($i++) $query .= \",\";\n");
	fprintf(fp,"\t\t$query .= $field_name;\n");
	fprintf(fp,"\t}\n");
	fprintf(fp,"\t$query .= \" FROM %s\";\n", table->name);
	fprintf(fp,"\tif (strlen($sort)) {\n");
	fprintf(fp,"\t\t$query .= \" ORDER BY \" . $sort;\n");
	fprintf(fp,"\t\tif (strlen($dir)) $query .= $dir;\n"); 
	fprintf(fp,"\t}\n");
	fprintf(fp,"\tif ($start > 0) {\n");
	fprintf(fp,"\t\t$query .= \" LIMIT \" . $start;\n");
	fprintf(fp,"\t\tif ($limit > 0) $query .= \",\" . $limit;\n");
	fprintf(fp,"\t} else if ($limit > 0)\n");
	fprintf(fp,"\t\t$query .= \" LIMIT \" . $limit;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\tdprintf(1,\"query: %%s\\n\",$query);\n");
#if MYSQL
	fprintf(fp,"\t$result = mysql_query($query);\n");
	fprintf(fp,"\tdprintf(1,\"result: %%s\\n\", $result);\n");
	fprintf(fp,"\tif ($result) {\n");
	fprintf(fp,"\t\t$data = array();\n");
	fprintf(fp,"\t\twhile ($row = mysql_fetch_assoc($result)) {\n");
	fprintf(fp,"\t\t\t$data[] = $row;\n");
	fprintf(fp,"\t}\n");
	fprintf(fp,"\tif ($debug >= 3) var_dump($data);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => true, \"total\" => count($data), \"data\" => $data));\n");
	fprintf(fp,"\t} else {\n");
	fprintf(fp,"\t\t$errmsg = mysql_error();\n");
	fprintf(fp,"\t\tdprintf(1,\"errmsg: %%s\\n\",$errmsg);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => false, \"message\" => $errmsg));\n");
	fprintf(fp,"\t}\n");
#else
	fprintf(fp,"\t$result = odbc_exec($db_conn,$query);\n");
	fprintf(fp,"\tdprintf(1,\"result: %%s\\n\", $result);\n");
	fprintf(fp,"\tif ($result) {\n");
	fprintf(fp,"\t\t$data = array();\n");
	fprintf(fp,"\t\twhile(($rec = odbc_fetch_array($result)) != false) {\n");
	fprintf(fp,"\t\t\t$data[] = $rec;\n");
	fprintf(fp,"\t\t}\n");
	fprintf(fp,"\t\tif ($debug >= 3) var_dump($data);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => true, \"total\" => count($data), \"data\" => $data));\n");
	fprintf(fp,"\t} else {\n");
	fprintf(fp,"\t\t$errmsg = odbc_errormsg($db_conn);\n");
	fprintf(fp,"\t\tdprintf(1,\"errmsg: %%s\\n\",$errmsg);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => false, \"message\" => $errmsg));\n");
	fprintf(fp,"\t}\n");
	fprintf(fp,"\n");
#endif
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
//	fprintf(fp,"
}

static void write_insert(FILE *fp, TABLE *table, int decl) {
	fprintf(fp,"function do_create() {\n");
	fprintf(fp,"\tglobal $db_conn,$debug,$fields,$data;\n");
	fprintf(fp,"\n");

	fprintf(fp,"\t$query = \"INSERT INTO patch_actions (\";\n");
	fprintf(fp,"\t$i = 0;\n");
	fprintf(fp,"\tforeach(array_keys($fields) as $field_name) {\n");
	fprintf(fp,"\t\tif (!isset($data->$field_name)) continue;\n");
	fprintf(fp,"\t\tif ($i++) $query .= \",\";\n");
	fprintf(fp,"\t\t$query .= $field_name;\n");
	fprintf(fp,"\t}\n");
	fprintf(fp,"\t$query .= \") VALUES (\";\n");
	fprintf(fp,"\t$i = 0;\n");
	fprintf(fp,"\tforeach(array_keys($fields) as $field_name) {\n");
	fprintf(fp,"\t\tif (!isset($data->$field_name)) continue;\n");
	fprintf(fp,"\t\t$type = gettype($data->$field_name);\n");
	fprintf(fp,"\t\tif ($type == \"boolean\" && $data->$field_name != 1) $data->$field_name = 0;\n");
	fprintf(fp,"\t\tif ($i++) $query .= \",\";\n");
	fprintf(fp,"\t\tif ($type == \"string\") $query .= \"'\";\n");
	fprintf(fp,"\t\t$query .= mysql_real_escape_string($data->$field_name);\n");
	fprintf(fp,"\t\tif ($type == \"string\") $query .= \"'\";\n");
	fprintf(fp,"\t}\n");
	fprintf(fp,"\t$query .= \")\";\n");
	fprintf(fp,"\tdprintf(1,\"query: %%s\\n\", $query);\n");
#if MYSQL
	fprintf(fp,"\t$result = mysql_query($query);\n");
	fprintf(fp,"\tdprintf(1,\"result: %%s\\n\", $result);\n");
	fprintf(fp,"\tif ($result) {\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => true, \"total\" => count($data), \"data\" => $data));\n");
	fprintf(fp,"\t} else {\n");
	fprintf(fp,"\t\t$errmsg = mysql_error();\n");
	fprintf(fp,"\t\tdprintf(1,\"errmsg: %%s\\n\",$errmsg);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => false, \"message\" => $errmsg));\n");
	fprintf(fp,"\t}\n");
#else
	fprintf(fp,"\t$result = odbc_exec($db_conn,$query);\n");
	fprintf(fp,"\tdprintf(1,\"result: %%s\\n\", $result);\n");
	fprintf(fp,"\tif ($result) {\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => true, \"total\" => count($data), \"data\" => $data));\n");
	fprintf(fp,"\t} else {\n");
	fprintf(fp,"\t\t$errmsg = odbc_errormsg($db_conn);\n");
	fprintf(fp,"\t\tdprintf(1,\"errmsg: %%s\\n\",$errmsg);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => false, \"message\" => $errmsg));\n");
	fprintf(fp,"\t}\n");
#endif
	fprintf(fp,"};\n");
	fprintf(fp,"\n");
//	fprintf(fp,"\t
}

static void write_update(FILE *fp, TABLE *table, int decl) {
	COLUMN *field;

	fprintf(fp,"function do_update() {\n");
	fprintf(fp,"\tglobal $db_conn,$debug,$fields,$data;\n");
	fprintf(fp,"\n");
	list_reset(table->keys);
	field = list_get_next(table->keys);
	if (!field) {
		printf("error: php must have a key\n");
		exit(1);
	}
	fprintf(fp,"\t$query = \"UPDATE %s\";\n",table->name);
	fprintf(fp,"\t$i = 0;\n");
	fprintf(fp,"\tforeach(array_keys($fields) as $field_name) {\n");
	fprintf(fp,"\t\tif (!isset($data->$field_name)) continue;\n");
	fprintf(fp,"\t\tif ($i++) $query .= \",\";\n");
	fprintf(fp,"\t\telse $query .= \" SET\";\n");
	fprintf(fp,"\t\t$type = gettype($data->$field_name);\n");
	fprintf(fp,"\t$query .= \" \" . $field_name . \" = \";\n");
	fprintf(fp,"\t\tif ($type == \"string\") $query .= \"'\";\n");
	fprintf(fp,"\t\tif ($type == \"boolean\" && $data->$field_name != 1) $data->$field_name = 0;\n");
	fprintf(fp,"\t\t$query .= mysql_real_escape_string($data->$field_name);\n");
	fprintf(fp,"\t\tif ($type == \"string\") $query .= \"'\";\n");
	fprintf(fp,"\t}\n");
	fprintf(fp,"\t$query .= \" WHERE %s = \" . mysql_real_escape_string($data->%s);\n", field->name, field->name);
	fprintf(fp,"\tdprintf(1,\"query: %%s\\n\",$query);\n");
#if MYSQL
	fprintf(fp,"\t$result = mysql_query($query);\n");
	fprintf(fp,"\tdprintf(1,\"result: %%s\\n\", $result);\n");
	fprintf(fp,"\tif ($result) {\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => true, \"total\" => count($data), \"data\" => $data));\n");
	fprintf(fp,"\t} else {\n");
	fprintf(fp,"\t\t$errmsg = mysql_error();\n");
	fprintf(fp,"\t\tdprintf(1,\"errmsg: %%s\\n\",$errmsg);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => false, \"message\" => $errmsg));\n");
	fprintf(fp,"\t}\n");
#else
	fprintf(fp,"\t$result = odbc_exec($db_conn,$query);\n");
	fprintf(fp,"\tdprintf(1,\"result: %%s\\n\", $result);\n");
	fprintf(fp,"\tif ($result) {\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => true, \"total\" => count($data), \"data\" => $data));\n");
	fprintf(fp,"\t} else {\n");
	fprintf(fp,"\t\t$errmsg = odbc_errormsg($db_conn);\n");
	fprintf(fp,"\t\tdprintf(1,\"errmsg: %%s\\n\",$errmsg);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => false, \"message\" => $errmsg));\n");
	fprintf(fp,"\t}\n");
#endif
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
//	fprintf(fp,"\t
}

static void write_delete(FILE *fp, TABLE *table, int decl) {
	COLUMN *field;

	fprintf(fp,"function do_destroy() {\n");
	fprintf(fp,"\tglobal $db_conn,$debug,$fields,$data;\n");
	fprintf(fp,"\n");
	list_reset(table->keys);
	field = list_get_next(table->keys);
	if (!field) {
		printf("error: php must have a key\n");
		exit(1);
}
	fprintf(fp,"\t$query = \"DELETE FROM %s WHERE %s \";\n", table->name, field->name);
	fprintf(fp,"\tif (count($data) > 1) {\n");
	fprintf(fp,"\t\t$query .= \"IN (\";\n");
	fprintf(fp,"\t\t$x = 0;\n");
	fprintf(fp,"\t\tforeach ($data as $entry) {\n");
	fprintf(fp,"\t\t\tif ($x++) $query .= \",\";\n");
	fprintf(fp,"\t\t\t$query .= mysql_real_escape_string($entry);\n");
	fprintf(fp,"\t\t}\n");
	fprintf(fp,"\t\t$query .= \");\";\n");
	fprintf(fp,"\t} else {\n");
	fprintf(fp,"\t\t$query .= \"= \" . mysql_real_escape_string($data);\n");
	fprintf(fp,"\t}\n");
	fprintf(fp,"\tdprintf(1,\"query: %%s\\n\", $query);\n");
#if MYSQL
	fprintf(fp,"\t$result = mysql_query($query);\n");
	fprintf(fp,"\tdprintf(1,\"result: %%s\\n\", $result);\n");
	fprintf(fp,"\tif ($result) {\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => true, \"total\" => count($data), \"data\" => $data));\n");
	fprintf(fp,"\t} else {\n");
	fprintf(fp,"\t\t$errmsg = mysql_error();\n");
	fprintf(fp,"\t\tdprintf(1,\"errmsg: %%s\\n\",$errmsg);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => false, \"message\" => $errmsg));\n");
	fprintf(fp,"\t}\n");
#else
	fprintf(fp,"\t$result = odbc_exec($db_conn,$query);\n");
	fprintf(fp,"\tdprintf(1,\"result: %%s\\n\", $result);\n");
	fprintf(fp,"\tif ($result) {\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => true, \"total\" => count($data), \"data\" => $data));\n");
	fprintf(fp,"\t} else {\n");
	fprintf(fp,"\t\t$errmsg = odbc_errormsg($db_conn);\n");
	fprintf(fp,"\t\tdprintf(1,\"errmsg: %%s\\n\",$errmsg);\n");
	fprintf(fp,"\t\tprint json_encode(array(\"success\" => false, \"message\" => $errmsg));\n");
	fprintf(fp,"\t}\n");
#endif
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
//	fprintf(fp,"\t
}

int write_php_src(TABLE *table, char *funcs) {
	FILE *fp;
	COLUMN *field;
	char temp[128], *p, *p2;
	int first = 0;

	if (opts & OPT_DEBUG) printf("write_src: opening file: gendb_%s.c\n", table->name);
	sprintf(temp,"gendb_%s.php",table->name);
	fp = fopen(temp,"w+");
	if (!fp) {
		perror("fopen src");
		return 1;
	}

	/* Write the comment */
	fprintf(fp,"<?php\n");

//	fprintf(fp,"\n");
	fprintf(fp,"include \"db.inc\";\n");
	fprintf(fp,"\n");

	fprintf(fp,"$debug = 0;\n");
	fprintf(fp,"\n");

	/* Write out table structure */
	fprintf(fp,"$fields = array(\n");
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		dprintf("field: name: %s, type: %d\n", field->name, field->type);
		switch(field->type) {
		case FT_BIN:
		case FT_SHORT:
		case FT_INT:
		case FT_LONG:
		case FT_QUAD:
			p = "int";
			break;
		case FT_FLOAT:
		case FT_DOUBLE:
		case FT_LONGDBL:
			p = "float";
			break;
		case FT_DATE:
		case FT_STRING:
		case FT_CHAR:
		default:
			p = "string";
			break;
		}
		fprintf(fp,"\t\'%s\' => \'%s\',\n", field->name, p);
	}
	fprintf(fp,");\n");
	fprintf(fp,"\n");

	/* for CLI calls */
	fprintf(fp,"if (!isset($argv)) $argv=array();\n");
	fprintf(fp,"foreach ($argv as $arg) {\n");
	fprintf(fp,"\t$e=explode(\"=\",$arg);\n");
	fprintf(fp,"\tif(count($e)==2)\n");
	fprintf(fp,"\t\t$_REQUEST[$e[0]]=$e[1];\n");
	fprintf(fp,"\telse\n");
	fprintf(fp,"\t\t$_REQUEST[$e[0]]=0;\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");

	fprintf(fp,"if (!isset($_REQUEST[\"xaction\"])) {\n");
	fprintf(fp,"\tprint json_encode(array(\"success\" => false, \"message\" => \"invalid xaction\"));\n");
	fprintf(fp,"\texit;\n");
	fprintf(fp,"}\n");
        fprintf(fp,"$xaction = $_REQUEST[\"xaction\"];\n");
	fprintf(fp,"dprintf(1,\"xaction: %%s\\n\",$xaction);\n");
	fprintf(fp,"\n");

	fprintf(fp,"switch($xaction) {\n");
	if (strchr(funcs,'s')) {
		fprintf(fp,"case \"read\":\n");
		fprintf(fp,"\tbreak;\n");
	}
	if (strchr(funcs,'i') || strchr(funcs,'u') || strchr(funcs,'d')) {
		if (strchr(funcs,'i')) fprintf(fp,"case \"create\":\n");
		if (strchr(funcs,'u')) fprintf(fp,"case \"update\":\n");
		if (strchr(funcs,'d')) fprintf(fp,"case \"destroy\":\n");
//		fprintf(fp,"\t$debug=3;\n");
		fprintf(fp,"\tif (isset($_REQUEST[\"data\"]) && ($_REQUEST[\"data\"] != \"\")) {\n");
		fprintf(fp,"\t\t$jsonString = str_replace(\"\\\\\", \"\", $_REQUEST[\"data\"]);\n");
		fprintf(fp,"\t\tdprintf(1,\"jsonString: %%s\\n\", $jsonString);\n");
		fprintf(fp,"\t\t$data = json_decode($jsonString);\n");
		fprintf(fp,"\t\tif ($debug >= 3) var_dump($data);\n");
		fprintf(fp,"\t} else {\n");
		fprintf(fp,"\t\tprint json_encode(array(\"success\" => false, \"message\" => \"invalid request - no data\"));\n");
		fprintf(fp,"\t\texit;\n");
		fprintf(fp,"\t}\n");
		fprintf(fp,"\tbreak;\n");
	}
	fprintf(fp,"default:\n");
	fprintf(fp,"\tprint json_encode(array(\"success\" => false, \"message\" => \"invalid xaction\"));\n");
	fprintf(fp,"\tbreak;\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");

	/* Funcs: read/create/update/destroy */
	for(p = funcs; *p; p++) {
//		dprintf("func: %c\n", *p);
		switch(*p) {
		case 's': /* read */
			write_select(fp,table,0);
			break;
		case 'i': /* create */
			write_insert(fp,table,0);
			break;
		case 'u': /* update */
			write_update(fp,table,0);
			break;
		case 'd': /* destroy */
			write_delete(fp,table,0);
			break;
		}
	}

	fprintf(fp,"eval(\"do_$xaction();\");\n");

	/* Close the file and return */
	if (opts & OPT_DEBUG) printf("write_src: done!\n");
	fprintf(fp,"\n");
#if MYSQL
	fprintf(fp,"mysql_close($db_conn);\n");
#else
	fprintf(fp,"odbc_close($db_conn);\n");
#endif
	fprintf(fp,"?>\n");
	fclose(fp);
	return 0;
}
