
/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include <stdio.h>
#include <string.h>
#include "gendb_inv_uson_system_info.h"

int inv_uson_system_info_fetch(DB *db) {
#if inv_uson_system_info_field_count > 0
	SQLRETURN ret;

	dprintf("Fetching data...\n");
	ret = SQLFetch(db->hstmt);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	if (ret != 100) ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_uson_system_info_fetch");
	return ret;
#else
	return 0;
#endif
}

int inv_uson_system_info_fetch_record(DB *db, struct inv_uson_system_info_record *rec) {
#if inv_uson_system_info_field_count > 0
	SQLRETURN ret;

	if ((ret = inv_uson_system_info_fetch(db)) != 0) {
		SQLCloseCursor(db->hstmt);
		return ret;
	}
	memset(rec,0,sizeof(*rec));
	SQLGetData(db->hstmt, 1, SQL_C_BIGINT, (SQLPOINTER) &rec->_id, sizeof(rec->_id), 0);
	SQLGetData(db->hstmt, 2, SQL_C_BINARY, (SQLPOINTER) rec->_resourceguid, sizeof(rec->_resourceguid), 0);
	SQLGetData(db->hstmt, 3, SQL_C_CHAR, (SQLPOINTER) rec->environment, sizeof(rec->environment), 0);
	SQLGetData(db->hstmt, 4, SQL_C_CHAR, (SQLPOINTER) rec->criticality, sizeof(rec->criticality), 0);
	SQLGetData(db->hstmt, 5, SQL_C_CHAR, (SQLPOINTER) rec->primary_storage_type, sizeof(rec->primary_storage_type), 0);
	SQLGetData(db->hstmt, 6, SQL_C_CHAR, (SQLPOINTER) rec->primary_storage_description, sizeof(rec->primary_storage_description), 0);
	SQLGetData(db->hstmt, 7, SQL_C_CHAR, (SQLPOINTER) rec->notes, sizeof(rec->notes), 0);
	SQLGetData(db->hstmt, 8, SQL_C_CHAR, (SQLPOINTER) rec->warranty_type, sizeof(rec->warranty_type), 0);
	SQLGetData(db->hstmt, 9, SQL_C_CHAR, (SQLPOINTER) rec->warranty_information, sizeof(rec->warranty_information), 0);
	SQLGetData(db->hstmt, 10, SQL_C_CHAR, (SQLPOINTER) rec->start, sizeof(rec->start), 0);
	SQLGetData(db->hstmt, 11, SQL_C_CHAR, (SQLPOINTER) rec->end, sizeof(rec->end), 0);
	SQLGetData(db->hstmt, 12, SQL_C_CHAR, (SQLPOINTER) rec->monitoring, sizeof(rec->monitoring), 0);
	SQLGetData(db->hstmt, 13, SQL_C_CHAR, (SQLPOINTER) rec->monitoring_comments, sizeof(rec->monitoring_comments), 0);
	dprintf("----------------- inv_uson_system_info record -----------------\n");
	dprintf("_id                 :  %lld\n",rec->_id);
	dprintf("_resourceguid       :  %x\n",rec->_resourceguid);
	dprintf("environment         :  %s\n",rec->environment);
	dprintf("criticality         :  %s\n",rec->criticality);
	dprintf("primary_storage_type:  %s\n",rec->primary_storage_type);
	dprintf("primary_storage_desc:  %s\n",rec->primary_storage_description);
	dprintf("notes               :  %s\n",rec->notes);
	dprintf("warranty_type       :  %s\n",rec->warranty_type);
	dprintf("warranty_information:  %s\n",rec->warranty_information);
	dprintf("start               :  %s\n",rec->start);
	dprintf("end                 :  %s\n",rec->end);
	dprintf("monitoring          :  %s\n",rec->monitoring);
	dprintf("monitoring_comments :  %s\n",rec->monitoring_comments);
#endif
	return 0;
}

int inv_uson_system_info_select(DB *db, char *clause) {
#if inv_uson_system_info_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"SELECT inv_uson_system_info._id,inv_uson_system_info._resourceguid,inv_uson_system_info.environment,inv_uson_system_info.criticality,inv_uson_system_info.primary_storage_type,inv_uson_system_info.primary_storage_description,inv_uson_system_info.notes,inv_uson_system_info.warranty_type,inv_uson_system_info.warranty_information,inv_uson_system_info.start,inv_uson_system_info.end,inv_uson_system_info.monitoring,inv_uson_system_info.monitoring_comments FROM inv_uson_system_info %s",(clause ? clause : ""));
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_uson_system_info_select");
	return ret;
#else
	return 0;
#endif
}

int inv_uson_system_info_select_record(DB *db, struct inv_uson_system_info_record *rec, char *clause) {
#if inv_uson_system_info_field_count > 0
	SQLRETURN ret;

	if ((ret = inv_uson_system_info_select(db, clause)) != 0) return ret;
	if ((ret = inv_uson_system_info_fetch_record(db, rec)) != 0) return ret;
	SQLCloseCursor(db->hstmt);
#endif
	return 0;
}

int inv_uson_system_info_insert(DB *db, struct inv_uson_system_info_record *rec) {
#if inv_uson_system_info_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"INSERT INTO inv_uson_system_info (_id,_resourceguid,environment,criticality,primary_storage_type,primary_storage_description,notes,warranty_type,warranty_information,start,end,monitoring,monitoring_comments) VALUES (%lld,%x,'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')",rec->_id,rec->_resourceguid,rec->environment,rec->criticality,rec->primary_storage_type,rec->primary_storage_description,rec->notes,rec->warranty_type,rec->warranty_information,rec->start,rec->end,rec->monitoring,rec->monitoring_comments);
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_uson_system_info_insert");
	printf("Offending query: %s\n", query);
	return ret;
#else
	return 0;
#endif
}

int inv_uson_system_info_update_record(DB *db, struct inv_uson_system_info_record *rec, char *clause) {
#if inv_uson_system_info_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"UPDATE inv_uson_system_info SET _id = %lld,_resourceguid = %x,environment = '%s',criticality = '%s',primary_storage_type = '%s',primary_storage_description = '%s',notes = '%s',warranty_type = '%s',warranty_information = '%s',start = '%s',end = '%s',monitoring = '%s',monitoring_comments = '%s'",rec->_id,rec->_resourceguid,rec->environment,rec->criticality,rec->primary_storage_type,rec->primary_storage_description,rec->notes,rec->warranty_type,rec->warranty_information,rec->start,rec->end,rec->monitoring,rec->monitoring_comments);
	if (clause) {
		if (clause[0] != ' ') strcat(query," ");
		strcat(query,clause);
	}
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_uson_system_info_update_record");
	return ret;
#else
	return 0;
#endif
}

int inv_uson_system_info_delete(DB *db, char *clause) {
#if inv_uson_system_info_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"DELETE FROM inv_uson_system_info %s",(clause ? clause : ""));
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_uson_system_info_delete");
	return ret;
#else
	return 0;
#endif
}
