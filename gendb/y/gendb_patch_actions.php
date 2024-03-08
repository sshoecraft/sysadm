<?php
include "db.inc";

$debug = 0;

$fields = array(
	'id' => 'string',
	'enabled' => 'string',
	'time' => 'int',
	'prog' => 'string',
	'description' => 'string',
	'allci' => 'string',
	'os' => 'string',
	'dep' => 'string',
);

if (!isset($argv)) $argv=array();
foreach ($argv as $arg) {
	$e=explode("=",$arg);
	if(count($e)==2)
		$_REQUEST[$e[0]]=$e[1];
	else
		$_REQUEST[$e[0]]=0;
}

if (!isset($_REQUEST["xaction"])) {
	print json_encode(array("success" => false, "message" => "invalid xaction"));
	exit;
}
$xaction = $_REQUEST["xaction"];
dprintf(1,"xaction: %s\n",$xaction);

switch($xaction) {
case "read":
	break;
case "create":
case "update":
case "destroy":
	if (isset($_REQUEST["data"]) && ($_REQUEST["data"] != "")) {
		$jsonString = str_replace("\\", "", $_REQUEST["data"]);
		dprintf(1,"jsonString: %s\n", $jsonString);
		$data = json_decode($jsonString);
		if ($debug >= 3) var_dump($data);
	} else {
		print json_encode(array("success" => false, "message" => "invalid request - no data"));
		exit;
	}
	break;
default:
	print json_encode(array("success" => false, "message" => "invalid xaction"));
	break;
}

function do_read() {
	global $db_conn,$debug,$fields;

	if (isset($_REQUEST["sort"]))
		$sort = $_REQUEST["sort"];
	else
		$sort = "";
	if (isset($_REQUEST["dir"]))
		$dir = $_REQUEST["dir"];
	else
		$dir = "ASC";
	if (isset($_REQUEST["start"]) && $_REQUEST["start"] != "")
		$start = $_REQUEST["start"];
	else
		$start = -1;
	if (isset($_REQUEST["limit"]))
		$limit = $_REQUEST["limit"];
	else
		$limit = -1;

	$query = "SELECT ";
	$i = 0;
	foreach(array_keys($fields) as $field_name) {
		if ($i++) $query .= ",";
		$query .= $field_name;
	}
	$query .= " FROM patch_actions";
	if (strlen($sort)) {
		$query .= " ORDER BY " . $sort;
		if (strlen($dir)) $query .= $dir;
	}
	if ($start > 0) {
		$query .= " LIMIT " . $start;
		if ($limit > 0) $query .= "," . $limit;
	} else if ($limit > 0)
		$query .= " LIMIT " . $limit;

	dprintf(1,"query: %s\n",$query);
	$result = odbc_exec($db_conn,$query);
	dprintf(1,"result: %s\n", $result);
	if ($result) {
		$data = array();
		while(($rec = odbc_fetch_array($result)) != false) {
			$data[] = $rec;
		}
		if ($debug >= 3) var_dump($data);
		print json_encode(array("success" => true, "total" => count($data), "data" => $data));
	} else {
		$errmsg = odbc_errormsg($db_conn);
		dprintf(1,"errmsg: %s\n",$errmsg);
		print json_encode(array("success" => false, "message" => $errmsg));
	}

}

function do_create() {
	global $db_conn,$debug,$fields,$data;

	$query = "INSERT INTO patch_actions (";
	$i = 0;
	foreach(array_keys($fields) as $field_name) {
		if (!isset($data->$field_name)) continue;
		if ($i++) $query .= ",";
		$query .= $field_name;
	}
	$query .= ") VALUES (";
	$i = 0;
	foreach(array_keys($fields) as $field_name) {
		if (!isset($data->$field_name)) continue;
		$type = gettype($data->$field_name);
		if ($type == "boolean" && $data->$field_name != 1) $data->$field_name = 0;
		if ($i++) $query .= ",";
		if ($type == "string") $query .= "'";
		$query .= mysql_real_escape_string($data->$field_name);
		if ($type == "string") $query .= "'";
	}
	$query .= ")";
	dprintf(1,"query: %s\n", $query);
	$result = odbc_exec($db_conn,$query);
	dprintf(1,"result: %s\n", $result);
	if ($result) {
		print json_encode(array("success" => true, "total" => count($data), "data" => $data));
	} else {
		$errmsg = odbc_errormsg($db_conn);
		dprintf(1,"errmsg: %s\n",$errmsg);
		print json_encode(array("success" => false, "message" => $errmsg));
	}
};

function do_update() {
	global $db_conn,$debug,$fields,$data;

	$query = "UPDATE patch_actions";
	$i = 0;
	foreach(array_keys($fields) as $field_name) {
		if (!isset($data->$field_name)) continue;
		if ($i++) $query .= ",";
		else $query .= " SET";
		$type = gettype($data->$field_name);
	$query .= " " . $field_name . " = ";
		if ($type == "string") $query .= "'";
		if ($type == "boolean" && $data->$field_name != 1) $data->$field_name = 0;
		$query .= mysql_real_escape_string($data->$field_name);
		if ($type == "string") $query .= "'";
	}
	$query .= " WHERE id = " . mysql_real_escape_string($data->id);
	dprintf(1,"query: %s\n",$query);
	$result = odbc_exec($db_conn,$query);
	dprintf(1,"result: %s\n", $result);
	if ($result) {
		print json_encode(array("success" => true, "total" => count($data), "data" => $data));
	} else {
		$errmsg = odbc_errormsg($db_conn);
		dprintf(1,"errmsg: %s\n",$errmsg);
		print json_encode(array("success" => false, "message" => $errmsg));
	}
}

function do_destroy() {
	global $db_conn,$debug,$fields,$data;

	$query = "DELETE FROM patch_actions WHERE id ";
	if (count($data) > 1) {
		$query .= "IN (";
		$x = 0;
		foreach ($data as $entry) {
			if ($x++) $query .= ",";
			$query .= mysql_real_escape_string($entry);
		}
		$query .= ");";
	} else {
		$query .= "= " . mysql_real_escape_string($data);
	}
	dprintf(1,"query: %s\n", $query);
	$result = odbc_exec($db_conn,$query);
	dprintf(1,"result: %s\n", $result);
	if ($result) {
		print json_encode(array("success" => true, "total" => count($data), "data" => $data));
	} else {
		$errmsg = odbc_errormsg($db_conn);
		dprintf(1,"errmsg: %s\n",$errmsg);
		print json_encode(array("success" => false, "message" => $errmsg));
	}
}

eval("do_$xaction();");

odbc_close($db_conn);
?>
