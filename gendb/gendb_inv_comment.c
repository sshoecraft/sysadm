
/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include <stdio.h>
#include <string.h>
#include "gendb_inv_comment.h"

int inv_comment_fetch(DB *db) {
#if inv_comment_field_count > 0
	SQLRETURN ret;

	dprintf("Fetching data...\n");
	ret = SQLFetch(db->hstmt);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	if (ret != 100) ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_comment_fetch");
	return ret;
#else
	return 0;
#endif
}

int inv_comment_fetch_record(DB *db, struct inv_comment_record *rec) {
#if inv_comment_field_count > 0
	SQLRETURN ret;

	if ((ret = inv_comment_fetch(db)) != 0) {
		SQLCloseCursor(db->hstmt);
		return ret;
	}
	memset(rec,0,sizeof(*rec));
	SQLGetData(db->hstmt, 1, SQL_C_BIGINT, (SQLPOINTER) &rec->_id, sizeof(rec->_id), 0);
	SQLGetData(db->hstmt, 2, SQL_C_BINARY, (SQLPOINTER) rec->_resourceguid, sizeof(rec->_resourceguid), 0);
	SQLGetData(db->hstmt, 3, SQL_C_CHAR, (SQLPOINTER) rec->comment, sizeof(rec->comment), 0);
	dprintf("----------------- inv_comment record -----------------\n");
	dprintf("_id                 :  %lld\n",rec->_id);
	dprintf("_resourceguid       :  %x\n",rec->_resourceguid);
	dprintf("comment             :  %s\n",rec->comment);
#endif
	return 0;
}

int inv_comment_select(DB *db, char *clause) {
#if inv_comment_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"SELECT inv_comment._id,inv_comment._resourceguid,inv_comment.comment FROM inv_comment %s",(clause ? clause : ""));
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_comment_select");
	return ret;
#else
	return 0;
#endif
}

int inv_comment_select_record(DB *db, struct inv_comment_record *rec, char *clause) {
#if inv_comment_field_count > 0
	SQLRETURN ret;

	if ((ret = inv_comment_select(db, clause)) != 0) return ret;
	if ((ret = inv_comment_fetch_record(db, rec)) != 0) return ret;
	SQLCloseCursor(db->hstmt);
#endif
	return 0;
}

int inv_comment_insert(DB *db, struct inv_comment_record *rec) {
#if inv_comment_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"INSERT INTO inv_comment (_id,_resourceguid,comment) VALUES (%lld,%x,'%s')",rec->_id,rec->_resourceguid,rec->comment);
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_comment_insert");
	printf("Offending query: %s\n", query);
	return ret;
#else
	return 0;
#endif
}

int inv_comment_update_record(DB *db, struct inv_comment_record *rec, char *clause) {
#if inv_comment_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"UPDATE inv_comment SET _id = %lld,_resourceguid = %x,comment = '%s'",rec->_id,rec->_resourceguid,rec->comment);
	if (clause) {
		if (clause[0] != ' ') strcat(query," ");
		strcat(query,clause);
	}
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_comment_update_record");
	return ret;
#else
	return 0;
#endif
}

int inv_comment_delete(DB *db, char *clause) {
#if inv_comment_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"DELETE FROM inv_comment %s",(clause ? clause : ""));
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"inv_comment_delete");
	return ret;
#else
	return 0;
#endif
}
