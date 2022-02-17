<?php

function dprintf($level) {
        global $debug;

        $args = func_get_args();
        unset($args[0]);
        if ($debug >= $level) {
                $spec = $args[1]; unset($args[1]);
                vprintf($spec,$args);
        }
}

$db_name = "patching";
$db_user = "patching";
$db_pass = "";

if (0) {
$db_host = "localhost:3306";
$db_conn = mysql_connect($db_host, $db_user, $db_pass);
if (!$db_conn) {
        echo "Unable to connect to DB: " . mysql_error();
        exit;
}
if (!mysql_select_db($db_name)) {
        echo "Unable to select mydbname: " . mysql_error();
        exit;
}
} else {
$db_conn = odbc_connect($db_name, $db_user, $db_pass);
}
var_dump($db_conn);
#echo "connected!<br>";
?>
