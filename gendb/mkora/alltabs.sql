set heading off
set pagesize 0
--select distinct object_name from all_objects
--	where substr(object_name,1,4) = 'ALL_' AND
--	object_type = 'VIEW' and owner = 'SYS';
select distinct object_name from all_objects where
	object_name = 'ALL_CATALOG' AND object_name = 'ALL_TAB_COLUMNS';
quit;
